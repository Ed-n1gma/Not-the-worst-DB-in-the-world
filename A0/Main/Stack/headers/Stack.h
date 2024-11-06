
#ifndef STACK_H
#define STACK_H

// this is the node class used to build up the LIFO stack
#include <cstddef>
#include <cstdlib>
#include <memory>

template <class Data>
class Node {

private:

	Data holdMe;
	Node *next;
	
public:

	/*****************************************/
	/** WHATEVER CODE YOU NEED TO ADD HERE!! */
	/*****************************************/
	Node(Data val) : holdMe(val), next(nullptr) {};
	Data getVal() const { return this->holdMe; };
	Node<Data>* getNext() const { return this->next; };
	void setNext(Node<Data>* node) { this->next = node; };
};

// a simple LIFO stack
template <class Data> 
class Stack {

	Node <Data> *head;
	int size;

public:

	// destroys the stack
	~Stack () {
		Node<Data> *temp;
		while(head) {
			temp = head->getNext();
			delete head;
			head = temp;
			size--;
		}
	}

	// creates an empty stack
	Stack () : head(nullptr), size(0) { }

	Stack(const Stack<Data>& st) {
		this->size = st.length();
		if (this->size == 0) return;
		this->head = new Node<Data>(st.head->getVal());
		Node<Data> *p = this->head, *q = st.head->getNext();
		while (q) {
			p->setNext(new Node<Data>(q->getVal()));
			p = p->getNext();
			q = q->getNext();
		}
	}

	// adds pushMe to the top of the stack
	void push (Data val) {
		Node<Data> *newNode = new Node<Data>(val);
		newNode->setNext(head);
		head = newNode;
		size++;
	}

	// return true if there are not any items in the stack
	bool isEmpty () { return size == 0; }

	// pops the item on the top of the stack off, returning it...
	// if the stack is empty, the behavior is undefined
	Data pop () {
		Data ret = head->getVal();
		Node<Data>* temp = head->getNext();
		delete head;
		head = temp;
		size--;
		return ret;
	}

	int length() const {
		return size;
	}
};

#endif
