//queeueu class implemented to know what next pieces should be placed
class queue {
    private:
        const int maxSize = 4;
        int nextPcs[4];
        int front;          //Variable to know what index is front
        int size;           //variable to know how many numbers there are in the array
    public:
        queue();
        void push(const int& value);
        void pop();
        int& first();
        bool is_empty() const;
};

queue::queue() {
    front = 0;
    size = 0;
}

void queue::push(const int& value) {
    if (size == maxSize) {
        //Do nothing
    }
    else {
        //push new value at the end and increment size
        nextPcs[(front + size) % maxSize] = value;
        size++;
    }
}

 void queue::pop() {
    if (is_empty()) {
        //DO nothing since empty
    }
    else {
        //ignore current front and go next
        front = (front + 1) % maxSize;
        size--;
    }
  }

  int& queue::first() {
    if (is_empty()) {
        //DO nothing since empty
    }
    else {
        return nextPcs[front];
    }
  }

  bool queue::is_empty() const {
    return size == 0;
  }