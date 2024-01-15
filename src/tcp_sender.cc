#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */

// bool compareSeg(TCPSenderMessage& a, TCPSenderMessage& b) {
//     return a.seqno.unwrap(isn_, checkpoint)  < b.seqno.unwrap(isn_, checkpoint) ;
// }

/* Construct TCP sender with given default Retransmission Timeout and possible ISN */
TCPSender::TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn ) : isn_( fixed_isn.value_or( Wrap32 { std::random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms ){
  cur_RTO_ms = initial_RTO_ms_;
  seqno = isn_;
  window_size = 1;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return sequenceNumbersFli;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutiveRetrans;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
 
      // use another queue instead of sendedMessage?

      if(sendedMessage.has_value()){
        TCPSenderMessage returnMessage = sendedMessage.value();
        sendedMessage = optional<TCPSenderMessage>{};
        return returnMessage;
      }else{
        return sendedMessage;
      }

    //}
  



  
 

  /*
  // Your code here.
  TCPSenderMessage message;
  if(!isTimerRunning && !outstandingSeg.empty()){
    message = outstandingSeg.front();
   
    if(window_size > 0){
      consecutiveRetrans++;
      cur_RTO_ms *= 2;
    }

    isTimerRunning = true;
    timer = cur_RTO_ms;

  }else{
    //if(bs.reader().bytes_buffered() != 0){
      bool SYN = seqno == isn_;
      bool FIN = bs.reader().is_finished();
      cout << "SYN: " << SYN << " FIN: " << FIN << "stream_bytes: "  << bs.reader().bytes_buffered() << endl;
      if(bs.reader().bytes_buffered() == 0 && !SYN && !FIN){
        cout << "return empty" << endl;
        return optional<TCPSenderMessage>{};
      }
      uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
      uint64_t sendLen = min(bs.reader().bytes_buffered(), equalWindowSize);
      sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
      string data;
      cout << "reader address: " << &bs.reader() << endl;
      cout << "reader outer size before: " << bs.reader().bytes_buffered() << endl;
      // read( bs.reader(), sendLen, data );
      bs.reader().pop(3);
      bs.writer().push("a");
      cout << "reader outer size: " << bs.reader().bytes_buffered() << endl;
      
      message = {
        seqno,
        SYN,
        Buffer(data),
        FIN
      };
      seqno = seqno + message.sequence_length();
      sequenceNumbersFli += message.sequence_length();
      checkpoint += message.sequence_length();

      outstandingSeg.push_back(message);
      //outstandingSeg.sort(compareSeg);

      if(!isTimerRunning){
        isTimerRunning = true;
        timer = cur_RTO_ms;
      }
    //}
  }



  
  return message;
  */
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // bs.reader() = outbound_stream;

  bool SYN = seqno == isn_;
  bool FIN = outbound_stream.is_finished();
  cout << "SYN: " << SYN << " FIN: " << FIN << "stream_bytes: "  << outbound_stream.bytes_buffered() << endl;
  if(outbound_stream.bytes_buffered() == 0 && !SYN && !FIN){
    cout << "return empty" << endl;
    sendedMessage = optional<TCPSenderMessage>{};
    return;
  }
  uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
  uint64_t sendLen = min(outbound_stream.bytes_buffered(), equalWindowSize - SYN);
  sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
  if(window_size > 0){
    window_size -= sendLen + SYN;
  }
  // FIN = FIN && window_size > 0;
  // window_size -= FIN;

  string data;
  cout << "reader address: " << &outbound_stream << endl;
  cout << "reader outer size before: " << outbound_stream.bytes_buffered() << endl;
  read( outbound_stream, sendLen, data );
  // bs.reader().pop(3);
  // bs.writer().push("a");
  cout << "reader outer size: " << outbound_stream.bytes_buffered() << endl;
  
  TCPSenderMessage message = {
    seqno,
    SYN,
    Buffer(data),
    FIN
  };

  seqno = seqno + message.sequence_length();
  sequenceNumbersFli += message.sequence_length();
  checkpoint += message.sequence_length();

  outstandingSeg.push_back(message);
  //outstandingSeg.sort(compareSeg);

  if(!isTimerRunning){
    isTimerRunning = true;
    timer = cur_RTO_ms;
  }

  sendedMessage = message;



}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  TCPSenderMessage message;
 
  bool SYN = false;
  bool FIN = false;
 
  string data = "";
 
  
  message = {
    seqno,
    SYN,
    Buffer(data),
    FIN
  };

  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.

  bool isNewData = false;
  if(msg.ackno.has_value()){
    isNewData = true;
    for (auto it = outstandingSeg.begin(); it != outstandingSeg.end(); /* no increment here */) {
      if (msg.ackno.value().unwrap(isn_, checkpoint) >= it->seqno.unwrap(isn_, checkpoint) + it->sequence_length()) {
          sequenceNumbersFli -= it->sequence_length();
          it = outstandingSeg.erase(it);
      } else {
          ++it;
      }
    }
  }
  
  window_size = msg.window_size;

  
  // outstandingSeg.remove_if([ackno](TCPSenderMessage& messgae) { return ackno > messgae.seqno.WrappingInt32() + messgae.sequence_length();})
  if(outstandingSeg.empty()){
    isTimerRunning = false;
  }

  if(isNewData){
    cur_RTO_ms = initial_RTO_ms_;
    if(!outstandingSeg.empty()){
      isTimerRunning = true;
      timer = cur_RTO_ms;
    }
    consecutiveRetrans = 0;
  }

  cout << "list size: " << outstandingSeg.size() << endl;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer -= ms_since_last_tick;
  if(timer <= 0){
    if(isTimerRunning){
      sendedMessage = outstandingSeg.front();
   
      if(window_size > 0){
        consecutiveRetrans++;
        cur_RTO_ms *= 2;
      }

      timer = cur_RTO_ms;
    }
  }


  
  
}
