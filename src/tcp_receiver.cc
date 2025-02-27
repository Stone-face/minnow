#include "tcp_receiver.hh"
#include <cstdint>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if(message.SYN){
    ISN = message.seqno;  // ??
    isISNSet = true;
  }
  Wrap32 payloadSeqno = message.SYN ? message.seqno + 1 : message.seqno;
  uint64_t absSeqno = payloadSeqno.unwrap(ISN, checkpoint);
  string data = message.payload.release();
  reassembler.insert(absSeqno - 1, data, message.FIN, inbound_stream); // string_view??
  checkpoint = absSeqno - 1 + data.length() - 1; //  index of the last reassembled byte 
 
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  std::optional<Wrap32> ack;
  if(isISNSet){
    uint64_t ackNo = inbound_stream.bytes_pushed() + 1;  // +1: convert stream index to absolute index
    //  the first (unreceived)byte the receiver is interested in receiving
    ackNo += inbound_stream.is_closed() ? 1 : 0;
    ack = Wrap32::wrap(ackNo, ISN);
  }

  uint16_t windowSize = inbound_stream.available_capacity() > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(inbound_stream.available_capacity());

  TCPReceiverMessage message{
    ack,
    windowSize
  };
  
  return message;
}
