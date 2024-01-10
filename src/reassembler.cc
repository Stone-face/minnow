#include "reassembler.hh"


using namespace std;


void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  subPriorityQueue.push(Sub(first_index, data, is_last_substring));
  stored_bytes += data.length();


  while(!subPriorityQueue.empty() && subPriorityQueue.top().index <= ack_index){
    Sub popedSub = subPriorityQueue.pop();
    stored_bytes -= popedSub.data.length();

    if(popedSub.index + popedSub.data.length() > ack_index){
      string writedStr = popedSub.data.substr(ack_index - popedSub.index);
      output.push(writedStr);
      if(popedSub.is_last_substring){
        output.close();
      }
      ack_index = popedSub.index + popedSub.data.length();
    }
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return stored_bytes;
}
