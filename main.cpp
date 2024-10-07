#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <map>

using namespace std;

struct Node
{
	vector<void*> gate_input_to;
	vector<void*> gate_output_of;
	int value = -1;
	int ID = -1;
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

const char* gateNames[] = {
	"AND",
	"OR",
	"NOR",
	"NAND",
	"INV",
	"BUF",
	"XOR",
	"XNOR",
};

struct Gate
{
	vector<struct Node*> node_inputs;
	vector<struct Node*> node_outputs;
	enum GateType gType;
	bool resolved = false;
};

vector<Node*> nodeList;
vector<Gate*> gateList;

vector<int> inpNodesList;
vector<int> outNodesList;

vector<Gate*> unresolvedGates;

bool allOutputsAvailable = false;

map<int, Node*> nodeIDMapping;

bool doRemove(Gate* argGate)
{
	return argGate->resolved;
}

void processGateOutput()
{
	for (auto& g : unresolvedGates)
	{
		int output = 0;
		bool inpNotFound = false;

		switch (g->gType)
		{
		case AND:
			
			output = 1;
			
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output &= g->node_inputs[i]->value;
			}
			
			
				
			break;

		case OR:
			output = 0;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output |= g->node_inputs[i]->value;
			}

			break;

		case NAND:
			output = 0;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output |= !(g->node_inputs[i]->value);
			}
			
			break;

		case NOR:
			output = 1;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output &= !g->node_inputs[i]->value;
			}

			break;

		case INV:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			output = !g->node_inputs[0]->value;
			break;

		case BUF:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			output = g->node_inputs[0]->value;
			break;

		case XOR:
			output = 0;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output ^= g->node_inputs[i]->value;
			}
			break;

		case XNOR:
			output = 1;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->value == -1)
				{
					inpNotFound = true;
					break;
				}

				output ^= g->node_inputs[i]->value;
			}
			break;
		}

		if (!inpNotFound)
		{
			g->node_outputs[0]->value = output;
			g->resolved = true; //can be removed
		}
	}

	

	auto final = remove_if(unresolvedGates.begin(), unresolvedGates.end(), doRemove);
	unresolvedGates.erase(final, unresolvedGates.end());

	allOutputsAvailable = !(unresolvedGates.size());

}

int main(int argc, char* argv[])
{
	/* Parse input file and populate nodeList and gateList */
	ifstream inpFile(argv[1]);

	if (!inpFile.is_open())
	{
		return -EINVAL;
	}

	string inpLine;

	string token;

	while (getline(inpFile, inpLine))
	{
		stringstream inpFileEntry(inpLine);

		bool readingGate = false;
		bool readingInpNodes = false;
		bool readingOutNodes = false;

		while (inpFileEntry >> token)
		{
			/* Check if it's gate or input or output */
			if (token != "INPUT" && token != "OUTPUT" && !(readingGate || readingInpNodes || readingOutNodes))
			{
				/* It is a gate. */
				struct Gate* newGate = new struct Gate;
				readingGate = true;

				/* Figure out gate type */
				for (int i = 0; i < NUM_GATE_TYPE; i++)
				{
					if (gateNames[i] == token)
					{
						newGate->gType = (enum GateType)i;
					}
				}

				gateList.push_back(newGate);
				unresolvedGates.push_back(newGate);
			}
			else if (readingGate)
			{
				int ID = stoi(token);
				struct Node* newNode = new struct Node;
				if (nodeIDMapping.find(ID) != nodeIDMapping.end())
				{
					delete newNode;
					newNode = nodeIDMapping[ID];
					
				}
				else
				{
					nodeList.push_back(newNode);
					newNode->ID = ID;
					nodeIDMapping[ID] = newNode;
				}
				
				/* Read into input nodes list until we reach end of line. After loop ends, move last entry to output list */
				gateList.back()->node_inputs.push_back(newNode);	

			}
			else
			{
				/* It's an input or output line */
				if (token == "INPUT")
				{
					readingInpNodes = true;
				}
				else if (token == "OUTPUT")
				{
					readingOutNodes = true;
				}
				else if (readingInpNodes)
				{
					if (stoi(token) >= 0)
					{
						inpNodesList.push_back(stoi(token));
					}
				}
				else if (readingOutNodes)
				{
					if (stoi(token) >= 0)
					{
						outNodesList.push_back(stoi(token));
					}
				}
			}
			
		}

		if (readingGate)
		{
			/* Move last node to last gate's output list */

			gateList.back()->node_outputs.push_back(gateList.back()->node_inputs.back());

			gateList.back()->node_inputs.pop_back();

			readingGate = false;
		}

		readingInpNodes = false;
		readingOutNodes = false;
	}

	/* Parse input vector and assign values to nodes */
	for (int i = 0; i < strlen(argv[2]); i++)
	{
		/* Loop over the bits, find the node, and assign the value */
		for (auto& x : nodeList)
		{
			if (x->ID == inpNodesList[i])
			{
				x->value = argv[2][i]-'0';
			}
		}
	}

	/* Simulate and print output */
	while (!allOutputsAvailable)
	{
		processGateOutput();
	}

	for (auto n : outNodesList)
	{
		cout << nodeIDMapping[n]->value;
	}

	return 0;
}