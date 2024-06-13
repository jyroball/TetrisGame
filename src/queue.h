//queeueu class implemented to know what next pieces should be placed
class queue {
    private:
        //Variables for normal Queue
        const int maxSize = 4;  //Total number of pieces in
        int nextPcs[4];     //Variable for what the queue looks like
        int front;          //Variable to know what index is front
        int size;           //variable to know how many numbers there are in the array
    public:
        queue();
        //Queueu functions
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
    nextPcs[(front + size) % maxSize] = value;
    size++;
}

void queue::pop() {
    if (!is_empty()) {
        //ignore current front and go next
        front = (front + 1) % maxSize;
        size--;
    }
}

int& queue::first() {
    if (!is_empty()) {
        return nextPcs[front];
    }
}

bool queue::is_empty() const {
    return size == 0;
}