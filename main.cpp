#include <thread>
#include <unistd.h>
#include <iostream>
#include "monitor.h"

#define BUFFER_SIZE 1
#define TIMES 100000
#define MAX_SLEEP_TIME 300

template <class T>
class BufferFIFO : public Monitor {
    class Element {
    public:
        T value;
        bool readByA, readByB, readByC;    // used for synchronization [read by (Consumer) A/B/C]
        Element() : readByA(), readByB(), readByC() {} // initialise booleans to false
    };

    bool consumerA, consumerB, consumerC;  // used for stopping Consumers if they they already went through element
    Condition emptyCond, fullCond;         // used for waking Producer [fullCond] and Consumers [emptyCond]
    Condition consumerACond, consumerBCond, consumerCCond;
    unsigned scoreA, scoreB, scoreC;       // how many times Consumers consumed an element

    Element elements[BUFFER_SIZE];
    unsigned head, tail, length;

    void push(const Element &element) {
        elements[tail] = element;
        tail = (tail + 1) % BUFFER_SIZE;
        ++length;
    }

    Element pop() {
        Element popped = elements[head];
        head = (head + 1) % BUFFER_SIZE;
        --length;
        return popped;
    }

    void readA(Element &element) {
        element.readByA = true;
        consumerA = false;
        std::cout << "A read!" << std::endl;
    }

    void readB(Element &element) {
        element.readByB = true;
        consumerB = false;
        std::cout << "B read!" << std::endl;
    }

    void readC(Element &element) {
        element.readByC = true;
        consumerC = false;
        std::cout << "C read!" << std::endl;
    }

    void consumeA() {
        pop();
        ++scoreA;
        consumerA = true;
        std::cout << "Consumed by A!" << std::endl;
    }

    void consumeB() {
        pop();
        ++scoreB;
        consumerB = true;
        std::cout << "Consumed by B!" << std::endl;
    }

    void consumeC() {
        pop();
        ++scoreC;
        consumerC = true;
        std::cout << "Consumed by C!" << std::endl;
    }

    void wakeConsumerA() {
        consumerA = true;
        signal(consumerACond);
    }

    void wakeConsumerB() {
        consumerB = true;
        signal(consumerBCond);
    }

    void wakeConsumerC() {
        consumerC = true;
        signal(consumerCCond);
    }

public:
    BufferFIFO() : consumerA(true), consumerB(true), consumerC(true) {}

    void getByA() {
        enter();
        while(!consumerA)   // if Consumer A went through element, stop him (prevent active waiting)
            wait(consumerACond);
        while(length == 0)  // if there is nothing to read - stop
            wait(emptyCond);

        Element &element = elements[head];
        if(!element.readByB && !element.readByC) {
            readA(element);
            if(scoreB < scoreC)
                wakeConsumerB();
            wakeConsumerC();
        } else if((element.readByB || element.readByC) && (scoreA < scoreB)) {
            consumeA();
            wakeConsumerB();
            wakeConsumerC();
            signal(fullCond);
        } else {
            consumerA = false;
        }
        leave();
    }
    void getByB() {
        enter();
        while(!consumerB)
            wait(consumerBCond);
        while(length == 0)
            wait(emptyCond);

        Element &element = elements[head];
        if(!element.readByA && !element.readByC) {
            readB(element);
            if(scoreA < scoreB)
                wakeConsumerA();
            wakeConsumerC();
        } else if((element.readByA || element.readByC) && (scoreB < scoreC)) {
            consumeB();
            wakeConsumerA();
            wakeConsumerC();
            signal(fullCond);
        } else {
            consumerB = false;
        }
        leave();
    }
    void getByC() {
        enter();
        while(!consumerC)
            wait(consumerCCond);
        while(length == 0)
            wait(emptyCond);

        Element &element = elements[head];
        if(!element.readByA && !element.readByB && (scoreA < scoreB)) {
            readC(element);
            wakeConsumerA();
        } else if(!element.readByA && !element.readByB && (scoreB < scoreC)) {
            readC(element);
            wakeConsumerB();
        } else if(element.readByA || element.readByB) {
            consumeC();
            wakeConsumerA();
            wakeConsumerB();
            signal(fullCond);
        } else {
            consumerC = false;
        }
        leave();
    }

    void put(T value) {
        enter();
        if(length == BUFFER_SIZE) // if buffer is full - stop
            wait(fullCond);

        Element element;
        element.value = value;
        push(element);

        std::cout << "Scores: A: " << scoreA << " B: " << scoreB << " C: " << scoreC << std::endl;
        // wake waiting Consumers:
        signal(emptyCond);
        signal(emptyCond);
        signal(emptyCond);
        leave();
    }

};

BufferFIFO<int> *buffer;

template <typename T>
void runConsumerA() {
    int i = TIMES;
    while(--i) {
        buffer->getByA();
        usleep(rand() % MAX_SLEEP_TIME);
    }
}

template <typename T>
void runConsumerB() {
    int i = TIMES;
    while(--i) {
        buffer->getByB();
        usleep(rand() % MAX_SLEEP_TIME);
    }
}

template <typename T>
void runConsumerC() {
    int i = TIMES;
    while(--i) {
        buffer->getByC();
        usleep(rand() % MAX_SLEEP_TIME);
    }
}

template <typename T>
void runProducer() {
    int i = TIMES;
    while(--i) {
        buffer->put(i);
        usleep(rand() % MAX_SLEEP_TIME);
    }
}

int main() {
    srand(time(NULL));

    buffer = new BufferFIFO<int>();

    std::thread producer(runProducer<int>);
    std::thread consumerA(runConsumerA<int>);
    std::thread consumerB(runConsumerB<int>);
    std::thread consumerC(runConsumerC<int>);

    producer.join();
    consumerA.join();
    consumerB.join();
    consumerC.join();

    std::cout << "BYE!" << std::endl;
    return 0;
}
