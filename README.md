# Producer-Consumers problem

## Task description:
Given FIFO buffer of size N, with 1 producer and 3 consumers (A, B, C), implement following problem:

Producer pushes one element to the buffer if there is place for it. Given element can only be read once by the same consumer. Element, after being read by any two consumers, is removed from the buffer. Consumer that is removing the element (second reader) is given a point. Ensure that score of each consumer in order A, B, C is nondecreasing.
