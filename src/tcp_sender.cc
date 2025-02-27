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

      if(!sendedMessageQueue.empty()){
        TCPSenderMessage returnMessage = sendedMessageQueue.front();
        sendedMessageQueue.pop();
        return returnMessage;
      }else{
        return optional<TCPSenderMessage>{};
      }

    //}
  
 
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // bs.reader() = outbound_stream;
  while(true){
    uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
    if(static_cast<uint64_t>(equalWindowSize) <= sequenceNumbersFli){
      //cout << "return empty" << endl;
      return;
    }
    
    bool SYN = seqno == isn_;
    //bool FIN = outbound_stream.is_finished();
    //cout << "SYN: " << SYN <<  "stream_bytes: "  << outbound_stream.bytes_buffered() << endl;
    // uint64_t equalWindowSize = max(1UL, static_cast<uint64_t>(window_size));
    uint64_t availableSent =  equalWindowSize - sequenceNumbersFli;
    uint64_t sendLen = min(outbound_stream.bytes_buffered(), availableSent - SYN);
    sendLen = min(sendLen, TCPConfig::MAX_PAYLOAD_SIZE);
    // if(window_size > 0){
    //   window_size -= (sendLen + SYN);
    // }
    
    string data;
    //cout << "reader address: " << &outbound_stream << endl;
    //cout << "reader outer size before: " << outbound_stream.bytes_buffered() << endl;
    read( outbound_stream, sendLen, data );
    //cout << "reader outer size: " << outbound_stream.bytes_buffered() << endl;
    
    bool FIN = outbound_stream.is_finished() && availableSent - SYN - sendLen > 0 && !FINSent;
    FINSent |= FIN;
    //cout << "FIN: " << FIN << " window_size: " << window_size << endl;
    // window_size -= FIN;

    if(data.length() == 0 && !SYN && !FIN){
      //cout << "return empty" << endl;
      return;
    }

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

    sendedMessageQueue.push(message);
  }
  



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

  window_size = msg.window_size;
  
  bool isNewData = false;
  if(msg.ackno.has_value()){
    
    // ignore impossible ackno
    if(msg.ackno.value().unwrap(isn_, checkpoint) > seqno.unwrap(isn_, checkpoint)){
      return;
    }

    
    for (auto it = outstandingSeg.begin(); it != outstandingSeg.end(); /* no increment here */) {
      if (msg.ackno.value().unwrap(isn_, checkpoint) >= it->seqno.unwrap(isn_, checkpoint) + it->sequence_length()) {
          sequenceNumbersFli -= it->sequence_length();
          it = outstandingSeg.erase(it);
          isNewData = true;
      } else {
          ++it;
      }
    }
  }
  
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

  //cout << "list size: " << outstandingSeg.size() << endl;
  //cout << "sequenceNumbersFli " << sequenceNumbersFli << endl;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  
  if(timer < ms_since_last_tick){
    timer = 0;
  }else{
    timer -= ms_since_last_tick;
  }

  //cout << "timer: " << timer << " isRunning: " << isTimerRunning << endl;
  if(timer == 0){
    if(isTimerRunning){
      TCPSenderMessage message = outstandingSeg.front();
      sendedMessageQueue.push(message);
      //cout << "resent message seqno: " << message.seqno.WrappingInt32() << endl;
      if(window_size > 0){
        consecutiveRetrans++;
        cur_RTO_ms *= 2;
      }

      timer = cur_RTO_ms;
    }
  }


  
  
}
