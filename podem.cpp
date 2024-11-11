#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <list>

using namespace std;

struct Node
{
	vector<void*> gate_input_to;
	vector<void*> gate_output_of;
	int value = -1;
	int ID = -1;
	int analysed = -1;
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

enum GateValue {
	VAL_ZERO,
	VAL_ONE,
	VAL_D,
	VAL_DBAR,
	VAL_X
};

enum GateValue gateOutputLookupAnd[5][5];
enum GateValue gateOutputLookupOr[5][5];
enum GateValue gateOutputLookupNand[5][5];
enum GateValue gateOutputLookupNor[5][5];
enum GateValue gateOutputLookupInv[5];
enum GateValue gateOutputLookupBuf[5];
enum GateValue gateOutputLookupXor[5][5];
enum GateValue gateOutputLookupXnor[5][5];

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

vector<string> allFaults;

map<int, set<int>> idFaultMapping;

int stuckAtFault; // Fault for which we need to find the test vector
int stuckAtValue; // The value that the fault at the above index si stuck at

/* Define the D frontier in a list */
list<Gate*> dFrontier;

void prepareGateLookup()
{
	// AND gate
	for (int i = 0; i <= 4; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if (i == 0 || j == 0)
			{
				gateOutputLookupAnd[i][j] = VAL_ZERO;
			}
			else
			{
				if (i == 1) { gateOutputLookupAnd[i][j] = (GateValue)j; }
				else if (j == 1) { gateOutputLookupAnd[i][j] = (GateValue)i; }
				else if (i == VAL_X || j == VAL_X) { gateOutputLookupAnd[i][j] = VAL_X; }
				else if (i == j) { gateOutputLookupAnd[i][j] = (GateValue)i; }
				else
				{
					/* The inputs have to be amongst D and Dbar */
					gateOutputLookupAnd[i][j] = VAL_ZERO;
				}



			}
		}
	}

	// OR gate
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if (i == 1 || j == 1)
			{
				gateOutputLookupOr[i][j] = VAL_ONE;
			}
			else
			{
				if (i == 0) { gateOutputLookupOr[i][j] = (GateValue)j; }
				else if (j == 0) { gateOutputLookupOr[i][j] = (GateValue)i; }
				else if (i == VAL_X || j == VAL_X) { gateOutputLookupOr[i][j] = VAL_X; }
				else if (i == j) { gateOutputLookupOr[i][j] = (GateValue)i; }
				else
				{
					/* The inputs have to be amongst D and Dbar */
					gateOutputLookupOr[i][j] = VAL_ONE;
				}


			}
		}
	}

	// INV and buffer
	for (int i = 0; i < 5; i++)
	{
		gateOutputLookupBuf[i] = (GateValue)i;

		if (i == VAL_X)
		{
			gateOutputLookupInv[i] = VAL_X;
		}
		else
		{
			if (i < 2) { gateOutputLookupInv[i] = (GateValue)(1 - i); }
			else
			{
				gateOutputLookupInv[i] = (GateValue)(5 - i);
			}
		}
	}

	// NAND and NOR
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			gateOutputLookupNand[i][j] = gateOutputLookupInv[gateOutputLookupAnd[i][j]];
			gateOutputLookupNor[i][j] = gateOutputLookupInv[gateOutputLookupOr[i][j]];
		}
	}
}

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

		int ret;

		set<int> tempFaultList;

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

				//output &= g->node_inputs[i]->value;
				output = gateOutputLookupAnd[g->node_inputs[i]->value][output];

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

				//output |= g->node_inputs[i]->value;
				output = gateOutputLookupOr[g->node_inputs[i]->value][output];
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

				//output |= !(g->node_inputs[i]->value);
				output = gateOutputLookupOr[gateOutputLookupInv[g->node_inputs[i]->value]][output];
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

				//output &= !g->node_inputs[i]->value;
				output = gateOutputLookupAnd[gateOutputLookupInv[g->node_inputs[i]->value]][output];
			}

			break;

		case INV:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			//output = !g->node_inputs[0]->value;
			output = gateOutputLookupInv[g->node_inputs[0]->value];

			break;

		case BUF:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			//output = g->node_inputs[0]->value;
			output = gateOutputLookupBuf[g->node_inputs[0]->value];

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

			g->node_outputs[0]->analysed = true;

		}
	}



	auto final = remove_if(unresolvedGates.begin(), unresolvedGates.end(), doRemove);
	unresolvedGates.erase(final, unresolvedGates.end());

	allOutputsAvailable = !(unresolvedGates.size());

}

void analyzeGateFaults()
{
	/* Find an unanalysed gate, whose all inputs have fault lists filled out */

}

void dFrontierSetup()
{
	//for (auto g : gateList)
	//{
	//	/* Just for the gates that have the faulty node as one of their inputs */
	//	if (find(g->node_inputs.begin(), g->node_inputs.end(), stuckAtFault) != g->node_inputs.end())
	//	{
	//		/* Add to the D frontier */
	//		dFrontier.push_back(g);
	//	}
	//}
}

pair<int, int> objective()
{
	// Initially you just need to check if the faulty node has its value set to the opp of the stuck at the value.
	// If not, then we just need to returnn as objective the node idx and the vaue we want, which is the stuck at value;s complement
	pair<int, int> result;

	// Assume that the fault under consideration is 
	if (nodeList[stuckAtFault]->value == -1)
	{
		// Just return the fault location and the complement of the stuck at value
		result.first = stuckAtFault;
		result.second = 1 - stuckAtValue;

		return result;
	}
	else
	{
		/* Apparently some assignment was able to justify the fault presence, or at least not contradict it */
		/* Now we just need to figure out the D frontier and pick a gate */
		Gate* dFrontGate = dFrontier.front();

		/* Choose an input to the dFront gate that is not the same ID as the fault node */
		for (auto gI : dFrontGate->node_inputs)
		{
			if (gI->ID != stuckAtFault) // This might not be sufficient - what if there's a fanout?? What'd be the D frontier in this case? Pretty sure it's just two or more gates at that point. 
			{
				
				result.first = gI->ID;

				// This needs to be set to a non ocntrolling value based on the gate of course
				switch (dFrontGate->gType)
				{
				case AND:
				case NAND:
					result.second = 1;
					break;

				case OR:
				case NOR:
					result.second = 0;
					break;

				case INV:
				case BUF:
					// TODO What do I do here??? Or if nothing, where do I handle the case where the gatee is either a Buffer or an inverter
					break;

				default:
					// TODO not sure if we need to handle the xor or the xnor case
					break;


				}

				return result;
			}
		}

	}

	return result;
}

int podem()
{
	/* Check error at PO */
	bool errorAtPO = false;
	for (auto n : outNodesList)
	{
		if (nodeIDMapping[n]->value == VAL_D || nodeIDMapping[n]->value == VAL_DBAR)
		{
			errorAtPO = true;
			break;
		}
	}

	if (errorAtPO)
	{
		return 0; //SUCCESS
	}
	
	/* Check for inconsistency at fault site */
	if (nodeList[stuckAtFault]->value != gateOutputLookupInv[stuckAtValue])
	{
		return -1; // FAILURE
	}
	/* Check if D frontier is empty. If we've reached here it means there's no error at PO anyway */
	if (dFrontier.empty())
	{
		return -1; // FAILURE
	}

	/* Get an objective */

	/* Backtrace and get a PI */

	/* Imply PI - no need to check that'd be done once you call PODEM subsequently */

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

				nodeList.back()->gate_input_to.push_back((void*)gateList.back());

				if (idFaultMapping.find(ID) != idFaultMapping.end())
				{
					//continue;
				}
				else
				{
					string idFault1 = to_string(ID) + " stuck at 0";

					string idFault2 = to_string(ID) + " stuck at 1";


					set<int> temp;

					temp.insert(allFaults.size());
					allFaults.push_back(idFault1);

					temp.insert(allFaults.size());
					allFaults.push_back(idFault2);

					//idFaultMapping[ID] = temp;
				}

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

			nodeList.back()->gate_input_to.pop_back();
			//nodeList.back()->gate_output_of.push_back((void *)gateList.back());

			readingGate = false;
		}

		readingInpNodes = false;
		readingOutNodes = false;
	}

	inpFile.close();

	// Prepare the loookup tables for all the gates
	prepareGateLookup();

	/* Parse input vector and assign values to nodes */
	for (int i = 0; i < strlen(argv[2]); i++)
	{
		/* Loop over the bits, find the node, and assign the value */
		for (auto& x : nodeList)
		{
			if (x->ID == inpNodesList[i])
			{
				x->value = argv[2][i] - '0';

				/* Add faults */
				x->analysed = 1;
				string temp = to_string(x->ID) + " stuck at " + to_string(1 - x->value);

				auto it = find(allFaults.begin(), allFaults.end(), temp);

				auto index = distance(allFaults.begin(), it);

				idFaultMapping[x->ID].insert(index);
			}
		}
	}

	/* Simulate and print output */
	while (!allOutputsAvailable)
	{
		processGateOutput();
	}

	// Print out the values of the output nodes
	for (auto n : outNodesList)
	{
		cout << nodeIDMapping[n]->value;
	}

	//stuckAtFault = 0; //For now we'll just look at the fault at the start of the fautl list

	// Populate the D frontier based on just the stuck at fault as that's the only plac ewe have a faulty input that is a D or a Dbar
	//stuckAtValue = 1;

	/* Invoke the D setup setup method */
	//dFrontierSetup();

	/* Invoke the test simulator */
	//podem();


	return 0;
}