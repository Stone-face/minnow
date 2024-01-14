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
  ackno = isn_;
  window_size = UINT16_MAX;
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
    if(outbound_stream_.bytes_buffered() != 0){
      bool SYN = ackno == isn_;
      bool FIN = outbound_stream_.is_finished();
      uint64_t sendLen = std::min(outbound_stream_.bytes_buffered(), window_size);
      sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
      string data;
      read( outbound_stream_, sendLen, data );
      
      message = {
        ackno,
        SYN,
        Buffer(data),
        FIN
      };

      outstandingSeg.push_back(message);
      outstandingSeg.sort(compareSeg);

      if(!isTimerRunning){
        isTimerRunning = true;
        timer = cur_RTO_ms;
      }
    }
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
    ackno,
    SYN,
    Buffer(data),
    FIN
  };

  return message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.

  cur_RTO_ms = initial_RTO_ms_;
  ackno = msg.ackno.has_value() ? max(ackno, msg.ackno.value());
  window_size = msg.window_size;

  for (auto it = outstandingSeg.begin(); it != outstandingSeg.end();) /* no increment here */) {
      if (ackno >= it->seqno + it->sequence_length()) {
          it = myList.erase(it);
      } else {
          ++it;
      }
  }
  // outstandingSeg.remove_if([ackno](TCPSenderMessage& messgae) { return ackno > messgae.seqno.WrappingInt32() + messgae.sequence_length();})

  if(outstandingSeg.empty()){
    isTimerRunning = false;
  }esle{
    isTimerRunning = true;
    timer = cur_RTO_ms;
  }

  consecutiveRetrans = 0;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer -= ms_since_last_tick;
  if(timer <= 0){
    isTimerRunning = false;
  }
  
}
