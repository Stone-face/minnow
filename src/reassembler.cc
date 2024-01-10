#include "reassembler.hh"


using namespace std;

Sub::Sub(uint64_t index, const std::string& data, bool is_last_substring) : index(index), data(data), is_last_substring(is_last_substring) {}
bool Sub::operator<(const Sub& other) const {
    return index < other.index; // Order by index in descending order
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  uint64_t store_endIdx = ack_index + output.available_capacity();
  if(store_endIdx <= first_index){
    return;
  }

  uint64_t  store_len = min(store_endIdx - first_index, data.length());

  subPriorityQueue.push(Sub(first_index, data.substr(0, store_len), is_last_substring));
  stored_bytes += store_len;
  cout << "push into pq: " << data.substr(0, store_len) << endl;
  cout << "top index: " << subPriorityQueue.top().index << " ack_index: " << ack_index << endl;
  while(!subPriorityQueue.empty() && subPriorityQueue.top().index <= ack_index){
    Sub popedSub = subPriorityQueue.top();
    cout << "in loop: ack_index " << ack_index << endl;
    if(popedSub.index + popedSub.data.length() >= ack_index){
      int len = min(popedSub.data.length() - (ack_index - popedSub.index), output.available_capacity());
      // cout << "available_capacity: " << output.available_capacity() << endl;
      cout << "ack_index " << ack_index << endl;
      // cout << "endIdx " << endIdx << endl;
      string writedStr = popedSub.data.substr(ack_index - popedSub.index, len);
      output.push(writedStr);
      cout << "push string: " << writedStr << endl;
      if(popedSub.is_last_substring){
        output.close();
      }
      ack_index += writedStr.length();
    }

    stored_bytes -= popedSub.data.length();
    subPriorityQueue.pop();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return stored_bytes;
}
