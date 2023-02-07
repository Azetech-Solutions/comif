package in.azetech.comif;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.method.ScrollingMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import com.hoho.android.usbserial.driver.UsbSerialDriver;
import com.hoho.android.usbserial.driver.UsbSerialPort;
import com.hoho.android.usbserial.driver.UsbSerialProber;
import com.hoho.android.usbserial.util.SerialInputOutputManager;

import java.io.IOException;
import java.security.InvalidParameterException;
import java.util.EnumSet;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity implements SerialInputOutputManager.Listener {

    private TextView receiveText;
    private TextView baudRateText;
    private Button sendButton;
    private Button refreshButton;
    private EditText txMsgId;
    private EditText txDlc;
    private EditText txDataBytes;

    int baudRate = 9600;
    private BroadcastReceiver broadcastReceiver;
    private Handler mainLooper;

    boolean isSerialPortConnected = false;
    private SerialInputOutputManager usbIoManager;
    private UsbSerialPort usbSerialPort;
    private UsbPermission usbPermission = UsbPermission.Unknown;
    private UsbDevice connectedDevice;
    private UsbSerialDriver connectedDriver;
    private int connectedPortNum = 0;

    private static final int refreshInterval = 200; // msec
    private Runnable runnable;

    private void run() {
        if (!isSerialPortConnected)
            return;
        mainLooper.postDelayed(runnable, refreshInterval);
    }

    void start() {
        if (!isSerialPortConnected)
            return;
        run();
    }

    void stop() {
        mainLooper.removeCallbacks(runnable);
    }

    @Override
    public void onNewData(byte[] data) {
        mainLooper.post(() -> {
            receive(data);
        });
    }

    @Override
    public void onRunError(Exception e) {
        mainLooper.post(() -> {
            status("Connection lost: " + e.getMessage());
            disconnect();
        });
    }

    private enum UsbPermission { Unknown, Requested, Granted, Denied }

    private static final String INTENT_ACTION_GRANT_USB = BuildConfig.APPLICATION_ID + ".GRANT_USB";


    private void NumberChannelTransmit(short Length, Byte[] Data)
    {
        StringBuilder data = new StringBuilder("--> Tx :");
        byte[] dataBytes = new byte[Length];
        for (int i = 0; i < Length; i++)
        {
            data.append(" ").append(String.format("%02X", Data[i]));
            dataBytes[i] = Data[i];
        }
        status(data.toString());

        send(dataBytes);
    }

    @SuppressLint("DefaultLocale")
    private void NumberChannelErrorNotification(int Debug0, int Debug1)
    {
        status("--> Error : " + String.format("%d", Debug0) + " 0x" + String.format("%02X", Debug1));
    }

    private void NumberChannelRxCbk(byte Length, byte[] Data)
    {
        StringBuilder data = new StringBuilder("--> Rx :");
        for (int i = 0; i < Length; i++)
        {
            data.append(" ").append(String.format("%02X", Data[i]));
        }
        status(data.toString());
    }

    /* Creating the ComIf Channel */
    Channel numberChannel = new Channel("NumberChannel",
            Channel.ChannelType.Number,
            new Channel.TransmitFunctionCallback() {
                public void TransmitFunction(short Length, Byte[] Data)
                {
                    NumberChannelTransmit(Length, Data);
                }
            },
            new Channel.ErrorNotificationListener() {
                public void ErrorNotification(int Debug0, int Debug1)
                {
                    NumberChannelErrorNotification(Debug0, Debug1);
                }
            }
    );

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        receiveText = findViewById(R.id.console); // TextView performance decreases with number of spans
        receiveText.setTextColor(Color.BLACK); // set as default color to reduce number of spans
        receiveText.setMovementMethod(ScrollingMovementMethod.getInstance());

        baudRateText = findViewById(R.id.baudRateTextView);
        baudRateText.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                final String[] values = getResources().getStringArray(R.array.baud_rates);
                int pos = java.util.Arrays.asList(values).indexOf(String.valueOf(baudRate));
                AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
                builder.setTitle("Baud rate");
                builder.setSingleChoiceItems(values, pos, (dialog, which) -> {
                    baudRate = Integer.parseInt(values[which]);
                    baudRateText.setText(String.valueOf(baudRate) + " bps");
                    status("Baudrate Set to " + String.valueOf(baudRate) + " bps");
                    dialog.dismiss();
                });
                builder.create().show();
            }
        });

        txMsgId = findViewById(R.id.messageID);
        txDlc = findViewById(R.id.dlc);
        txDataBytes = findViewById(R.id.databytes);

        sendButton = findViewById(R.id.sendButton);
        sendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(!isSerialPortConnected) {
                    status("No device connected!");
                    return;
                }

                // Send the messages
                if(txMsgId.getText().toString().equals("") || txDlc.getText().toString().equals("") || txDataBytes.getText().toString().equals("")) {
                    status("Not enough inputs!");
                    return;
                }

                int id = Integer.parseInt(txMsgId.getText().toString());
                if((id > 255) || (id <= 0)) {
                    status("Invalid ID!");
                    return;
                }

                int dlc = Integer.parseInt(txDlc.getText().toString());
                if((dlc > 255) || (dlc <= 0)) {
                    status("Invalid DLC!");
                    return;
                }

                String hexString = txDataBytes.getText().toString().toUpperCase().replace(" ", "");
                byte[] dataBytes = hexStringToByteArray(hexString);

                //TxMessage txMessage = new TxMessage((byte)id, (byte)dlc);
                //System.arraycopy(dataBytes, 0, txMessage.Data, 0, dataBytes.length);
                //numberChannel.Transmit(txMessage);
            }
        });

        refreshButton = findViewById(R.id.refreshButton);
        refreshButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                txMsgId.setText("");
                txDlc.setText("");
                txDataBytes.setText("");
                Refresh();
            }
        });

        // On connecting the USB Devices, this app will be opened automatically.
        // So, try scanning the devices and check for the connectivity
        broadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if(INTENT_ACTION_GRANT_USB.equals(intent.getAction())) {
                    usbPermission = intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)
                            ? UsbPermission.Granted : UsbPermission.Denied;
                    Connect();
                }
            }
        };
        mainLooper = new Handler(Looper.getMainLooper());
        runnable = this::run; // w/o explicit Runnable, a new lambda would be created on each postDelayed, which would not be found again by removeCallbacks

        Refresh();
    }

    @Override
    public void onResume() {
        super.onResume();
        registerReceiver(broadcastReceiver, new IntentFilter(INTENT_ACTION_GRANT_USB));

        if(usbPermission == UsbPermission.Unknown || usbPermission == UsbPermission.Granted)
            mainLooper.post(this::Connect);
    }

    @Override
    public void onPause() {
        if(isSerialPortConnected) {
            status("disconnected");
            disconnect();
        }
        unregisterReceiver(broadcastReceiver);
        super.onPause();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if("android.hardware.usb.action.USB_DEVICE_ATTACHED".equals(intent.getAction())) {
            Toast.makeText(this, "USB Device Detected!", Toast.LENGTH_SHORT).show();
        }
        super.onNewIntent(intent);
    }

    private void status(String str) {
        receiveText.append(str + "\n");
    }

    private static int toByte(char c) {
        if (c >= '0' && c <= '9')
            return (c - '0');
        if (c >= 'A' && c <= 'F')
            return (c - 'A' + 10);
        if (c >= 'a' && c <= 'f')
            return (c - 'a' + 10);

        throw new InvalidParameterException("Invalid hex char '" + c + "'");
    }

    public static byte[] hexStringToByteArray(String hexString) {
        int length = hexString.length();
        byte[] buffer = new byte[length / 2];

        for (int i = 0; i < length; i += 2) {
            buffer[i / 2] = (byte) ((toByte(hexString.charAt(i)) << 4) | toByte(hexString
                    .charAt(i + 1)));
        }

        return buffer;
    }

    private final static char[] HEX_DIGITS = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

    public static String dumpHexString(byte[] array) {
        return dumpHexString(array, 0, array.length);
    }

    public static String dumpHexString(byte[] array, int offset, int length) {
        StringBuilder result = new StringBuilder();

        byte[] line = new byte[8];
        int lineIndex = 0;

        for (int i = offset; i < offset + length; i++) {
            if (lineIndex == line.length) {
                for (int j = 0; j < line.length; j++) {
                    if (line[j] > ' ' && line[j] < '~') {
                        result.append(new String(line, j, 1));
                    } else {
                        result.append(".");
                    }
                }

                result.append("\n");
                lineIndex = 0;
            }

            byte b = array[i];
            result.append(HEX_DIGITS[(b >>> 4) & 0x0F]);
            result.append(HEX_DIGITS[b & 0x0F]);
            result.append(" ");

            line[lineIndex++] = b;
        }

        for (int i = 0; i < (line.length - lineIndex); i++) {
            result.append("   ");
        }
        for (int i = 0; i < lineIndex; i++) {
            if (line[i] > ' ' && line[i] < '~') {
                result.append(new String(line, i, 1));
            } else {
                result.append(".");
            }
        }

        return result.toString();
    }

    private void Refresh() {
        UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        UsbSerialProber usbDefaultProber = UsbSerialProber.getDefaultProber();
        for(UsbDevice device : usbManager.getDeviceList().values()) {
            UsbSerialDriver driver = usbDefaultProber.probeDevice(device);
            if(driver != null) {
                for(int port = 0; port < driver.getPorts().size(); port++) {
                    status("Connecting to " + driver.getClass().getSimpleName().replace("SerialDriver","")+", Port "+ String.valueOf(port));
                    status(String.format(Locale.US, "Vendor %04X, Product %04X", device.getVendorId(), device.getProductId()));
                }
                connectedDevice = device;
                connectedDriver = driver;
                connectedPortNum = 0; // Always select the first port

                Connect();
                break;
            } else {
                status("Driver not found for this device!");
                connectedDevice = null;
                connectedDriver = null;
                connectedPortNum = 0;
            }
        }
    }

    /* Connect to USB Devices */
    private void Connect() {
        if((connectedDevice != null) && (connectedDriver != null)) {

            UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
            usbSerialPort = connectedDriver.getPorts().get(connectedPortNum);

            UsbDeviceConnection usbConnection = usbManager.openDevice(connectedDriver.getDevice());
            if(usbConnection == null && usbPermission == UsbPermission.Unknown && !usbManager.hasPermission(connectedDriver.getDevice())) {
                usbPermission = UsbPermission.Requested;
                int flags = Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0;
                PendingIntent usbPermissionIntent = PendingIntent.getBroadcast(this, 0, new Intent(INTENT_ACTION_GRANT_USB), flags);
                usbManager.requestPermission(connectedDriver.getDevice(), usbPermissionIntent);
                return;
            }
            if(usbConnection == null) {
                if (!usbManager.hasPermission(connectedDriver.getDevice()))
                    status("Connection failed: permission denied");
                else
                    status("Connection failed: open failed");
                return;
            }

            try {
                if(!usbSerialPort.isOpen()) {
                    usbSerialPort.open(usbConnection);
                }
                usbSerialPort.setParameters(baudRate, 8, 1, UsbSerialPort.PARITY_NONE);
                usbIoManager = new SerialInputOutputManager(usbSerialPort, this);
                usbIoManager.start();

                status("Connected");
                isSerialPortConnected = true;
                start();
            } catch (Exception e) {
                status("Serial Port Open failed: " + e.getMessage());
                disconnect();
            }

        } else {
            status("No device connected!");
        }
    }

    private void disconnect() {
        isSerialPortConnected = false;
        stop();

        if(usbIoManager != null) {
            usbIoManager.setListener(null);
            usbIoManager.stop();
        }
        usbIoManager = null;
        try {
            usbSerialPort.close();
        } catch (IOException ignored) {}
        usbSerialPort = null;
    }

    private void send(byte[] dataBytes) {
        if(!isSerialPortConnected) {
            status("Not connected to a device!");
            return;
        }
        try {
            usbSerialPort.write(dataBytes, 2000); // 2000ms
        } catch (Exception e) {
            onRunError(e);
        }
    }

    private void receive(byte[] data) {
        if(data.length > 0) {
            status(dumpHexString(data));
        }
    }
}
