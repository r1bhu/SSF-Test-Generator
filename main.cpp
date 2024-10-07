#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

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
};

vector<Node*> nodeList;
vector<Gate*> gateList;

vector<int> inpNodesList;
vector<int> outNodesList;

void processGateOutput(struct Gate *gate)
{

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
			}
			else if (readingGate)
			{
				/* We're reading a node. */
				struct Node* newNode = new struct Node;

				int ID = stoi(token);
				newNode->ID = ID;

				/* Read into input nodes list until we reach end of line. After loop ends, move last entry to output list */
				gateList.back()->node_inputs.push_back(newNode);

				nodeList.push_back(newNode);

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

			gateList.back()->node_outputs.push_back(nodeList.back());

			gateList.back()->node_inputs.pop_back();

			//nodeList.back()->gate_input_to.pop_back();
			//nodeList.back()->gate_output_of.push_back((void *)gateList.back());

			readingGate = false;
		}

		readingInpNodes = false;
		readingOutNodes = false;
	}

	/* Parse input vector and assign values to nodes */

	/* Simulate and print output */

	cout << "Number of inputs: " << inpNodesList.size() << endl;
	cout << "Number of outputs: " << outNodesList.size() << endl;

	return 0;
}