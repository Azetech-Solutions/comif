import * as ComIf from './ComIf.mjs';

function ComIfChannelTransmit(Length, str)
{
    console.log("--> Tx : " + str);
}

function ComIfChannelErrorNotification(Debug0, Debug1)
{
    console.log(`--> Error : ${Debug0} 0x${Debug1.toString(16)}`);
}

function ComIfChannelRxCbk(Length, Data)
{
    console.log("--> Rx : " + Data.toString());
}

export const handler = async(event) => {
    
    var channel = new ComIf.Channel("myChannel", ComIf.StringChannel, ComIfChannelTransmit, ComIfChannelErrorNotification);
    
    // Creating and Sending a Tx Message
    var txMessage = new ComIf.TxMessage(0x12, 8);
    channel.Transmit(txMessage);
    
    // Creating and Registering an Rx Message
    var rxMessage = new ComIf.RxMessage(0x13, 8, ComIfChannelRxCbk);
    channel.RegisterRxMessage(rxMessage);

    // Sending an Rx Message to the channel
    var RxDataBytes = [0x7B, 0x13, 0x08, 1, 2, 3, 4, 5, 6, 7, 8, 0xDC, 0x7D ];
    channel.RxIndication_Bytes(RxDataBytes);
    
    const response = {
        statusCode: 200,
        body: JSON.stringify('Hello from Lambda!'),
    };
    return response;
};

