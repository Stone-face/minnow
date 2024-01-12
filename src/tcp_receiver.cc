#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if(message.SYN){
    ISN = message.seqno;  // ??
  }
  Wrap32 payloadSeqno = ISN ? message.seqno + 1 : message.seqno;
  reassembler.insert(payloadSeqno.unwrap(ISN, checkpoint) - 1, message.payload.string_view(), message.FIN, inbound_stream); // string_view??
  checkpoint = payloadSeqno.unwrap(ISN, checkpoint) - 1 + message.payload.string_view().length() - 1;
 
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  std::optional<Wrap32> ack;
  if(ISN){
    ack = Wrap32.wrap(inbound_stream.bytes_pushed() + 1, ISN);
  }


  TCPReceiverMessage message{
    ack,
    Writer.available_capacity()
  };
  
  return message;
}
