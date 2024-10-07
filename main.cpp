#include <iostream>
#include <stdio.h>
#include <vector>

using namespace std;

struct Node
{
	vector<void*> gate_input_to;
	vector<void*> gate_output_of;
	bool value;
};

enum GateType
{
	AND,
	OR,
	NOR,
	NAND,
	INV,
	BUF,
	XOR,
	XNOR,
	NUM_GATE_TYPE
};

struct Gate
{
	vector<struct Node*> node_inputs;
	vector<struct Node*> node_outputs;
	enum GateType gType;
};

vector<Node*> nodeList;
vector<Gate*> gateList;

int main(int argc, char* argv[])
{
	/* Parse input file and populate nodeList and gateList */

	/* Parse input vector and assign values to nodes */

	/* Simulate and print output */

	return 0;
}