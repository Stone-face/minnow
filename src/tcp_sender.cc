#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */

bool compareSeg(TCPSenderMessage& a, TCPSenderMessage& b) {
    return a.seqno.WrappingInt32()  < b.seqno.WrappingInt32() ;
}
/* Construct TCP sender with given default Retransmission Timeout and possible ISN */
TCPSender::TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn ) : isn_( fixed_isn.value_or( Wrap32 { std::random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms ){
  cur_RTO_ms = initial_RTO_ms_;
  seqno = isn_;
  window_size = 1;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return outstandingSeg.size();
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return consecutiveRetrans;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
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
    //if(outbound_stream_.bytes_buffered() != 0){
      bool SYN = seqno == isn_;
      bool FIN = outbound_stream_.is_finished();
      cout << "SYN: " << SYN << " FIN: " << FIN << "stream_bytes: "  << outbound_stream_.bytes_buffered() << endl;
      if(outbound_stream_.bytes_buffered() == 0 && !SYN && !FIN){
        cout << "return empty" << endl;
        return optional<TCPSenderMessage>{};
      }
      uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
      uint64_t sendLen = min(outbound_stream_.bytes_buffered(), equalWindowSize);
      sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
      string data;
      read( outbound_stream_, sendLen, data );
      
      message = {
        seqno,
        SYN,
        Buffer(data),
        FIN
      };
      seqno = seqno + message.sequence_length();
      outstandingSeg.push_back(message);
      outstandingSeg.sort(compareSeg);

      if(!isTimerRunning){
        isTimerRunning = true;
        timer = cur_RTO_ms;
      }
    //}
  }



  
  return message;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  outbound_stream_ = outbound_stream;
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
      if (msg.ackno.value().WrappingInt32() >= it->seqno.WrappingInt32() + it->sequence_length()) {
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
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer -= ms_since_last_tick;
  if(timer <= 0){
    isTimerRunning = false;
  }
  
}
