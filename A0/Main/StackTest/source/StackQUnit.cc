
#ifndef STACK_TEST_CC
#define STACK_TEST_CC

#include "QUnit.h"
#include "Stack.h"
#include <memory>
#include <iostream>
#include <vector>
#include <string>

int main () {

	QUnit::UnitTest qunit(std :: cerr, QUnit :: verbose);
	
	// UNIT TEST 1
	// just make sure we can create an empty stack, and that it is empty
	{
		
		Stack <int> myStack;
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);

	}

	// UNIT TEST 2
	// make sure we can push/pop a bunch of items
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		// now, pop everything off
		for (int i = 0; i < 10; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}
	}

	// UNIT TEST 3
	// make sure that the destructor works correctly... unless the stack correctly deallocates
	// all of the items it contains, this test will fail
	{
		static int temp = 0;
		struct Tester {
			Tester () {temp++;}
			~Tester () {temp--;}
		};

		{
			Stack <std :: shared_ptr <Tester>> myStack;
			std :: shared_ptr <Tester> myGuy = std :: make_shared <Tester> ();
			for (int i = 0; i < 10; i++) {
				myStack.push (myGuy);
			}
		}

		QUNIT_IS_EQUAL (temp, 0);
	}

	// UNIT TEST 4
	// make sure we can push/pop a bunch of items and then do it again
	{

		Stack <int> myStack;
		for (int i = 0; i < 10; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 5; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 9 - i);
		}

		for (int i = 5; i < 20; i++) {
			myStack.push (i);
		}

		for (int i = 0; i < 20; i++) {
			QUNIT_IS_EQUAL (myStack.pop (), 19 - i);
		}
		
		QUNIT_IS_EQUAL (myStack.isEmpty (), true);
	}

	// UNIT TEST 5
	// string test
	{
		Stack<std::string> myStack;
		std::vector<std::string> strs = {
			"abcd",
			"bnca",
			"adwd",
			"dawda",
			"adwdad"
		};

		for (std::string s : strs) {
			myStack.push(s);
		}
		for (int i = strs.size() - 1; i >= 0; i--) {
			QUNIT_IS_EQUAL (myStack.pop(), strs[i]);
		}

		QUNIT_IS_EQUAL (myStack.isEmpty (), true);
	}

	// UNIT TEST 6
	// const char* string test
	{
		const char* strs[5];
		strs[0] = "abdse";
		strs[1] = "awdafawdadw";
		strs[2] = "seffsefsd";
		strs[3] = "21314";
		strs[4] = "2ewda2r";

		Stack<const char*> myStack;

		for(int i = 0; i < 5; i++) myStack.push(strs[i]);

		for (int i = 4; i >= 0; i--) {
			QUNIT_IS_EQUAL (myStack.pop(), strs[i]);
		}

		QUNIT_IS_EQUAL (myStack.isEmpty (), true);
	}

	// UNIT TEST 7
	// stack of stacks
	{
		static int temp = 0;
		struct Tester {
			Tester () {temp++;}
			~Tester () {temp--;}
		};
		{
			Stack<std::shared_ptr<Tester>> inStack;
			Stack<Stack<std::shared_ptr<Tester>>> myStack;
			
			std::shared_ptr<Tester> myGuy = std::make_shared<Tester>();
			
			for (int i = 0; i < 10; i++) {
				inStack.push (myGuy);
			}
			myStack.push(inStack);
		}
		QUNIT_IS_EQUAL (temp, 0);
	}
	
}

#endif

