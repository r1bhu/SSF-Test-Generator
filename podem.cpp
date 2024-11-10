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

enum GateValue[] = {
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
enum GateValue gateOutputLookupBuf[5][5];
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
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			if (i == 0 || j == 0)
			{
				gateOutputLookupAnd[i][j] = VAL_ZERO;
			}
			else
			{
				if (i == 1) { gateOutputLookupAnd[i][j] = j; }
				else if (j == 1) { gateOutputLookupAnd[i][j] = i; }
				else if (i == VAL_X || j == VAL_X) { gateOutputLookupAnd[i][j] = VAL_X; }
				else if (i == j) { gateOutputLookupAnd[i][j] = i; }
				else
				{
					/* The inputs have to be amongst D and Dbar */
					gateOutputLookupAnd[i][j] = 0;
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
				if (i == 0) { gateOutputLookupOr[i][j] = j; }
				else if (j == 0) { gateOutputLookupOr[i][j] = i; }
				else if (i == VAL_X || j == VAL_X) { gateOutputLookupOr[i][j] = VAL_X; }
				else if (i == j) { gateOutputLookupOr[i][j] = i; }
				else
				{
					/* The inputs have to be amongst D and Dbar */
					gateOutputLookupOr[i][j] = 1;
				}


			}
		}
	}

	// INV and buffer
	for (int i = 0; i < 5; i++)
	{
		gateOutputLookupBuf[i] = i;

		if (i == VAL_X)
		{
			gateOutputLookupInv = VAL_X;
		}
		else
		{
			if (i < 2) { gateOutputLookupInv[i] = 1 - i; }
			else
			{
				gateOutputLookupInv[i] = 5 - i;
			}
		}
	}

	// NAND and NOR
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			gateLookupNand[i][j] = gateLookupInv[gateLookupAnd[i][j]];
			gateLookupNor[i][j] = gateLookupInv[gateLookupOr[i][j]];
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

				output &= g->node_inputs[i]->value;

			}

			if (!inpNotFound)
			{

				/* Add fault lists */
				if (output == 0)
				{
					set<int> removeList;
					set<int> intersectSet;
					bool encounteredLowInp = false;
					/* All faults causing values to non-controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{
						if (g->node_inputs[i]->value == 0)
						{

							if (!encounteredLowInp)
							{
								tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());
								encounteredLowInp = true;
							}
							else
							{
								set_intersection(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end(),
									tempFaultList.begin(), tempFaultList.end(), std::inserter(intersectSet, intersectSet.begin()));

								/* Clear tempFault list and transfer to tempFaultList */
								tempFaultList.clear();

								tempFaultList = intersectSet;

								/* Clear intersect */
								intersectSet.clear();
							}

						}
						else
						{

							for (int x : idFaultMapping[g->node_inputs[i]->ID])
							{
								removeList.insert(x);
								tempFaultList.erase(x);
							}
						}

					}
					for (int x : removeList)
					{
						tempFaultList.erase(x);
					}

				}
				else
				{
					/* All faults causing values to controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{
						tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

					}
				}

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

			if (!inpNotFound)
			{
				/* Add fault lists */
				if (output == 1)
				{
					set<int> removeList;
					set<int> intersectSet;
					bool encounteredHighInp = false;
					/* All faults causing values to non-controlling values. Need to find the intersection of faults that flip to 0. */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{

						if (g->node_inputs[i]->value == 1)
						{
							//tempFaultList.insert(idFaultMapping[nodeList[i].ID].begin(), idFaultMapping[nodeList[i].ID].end());
							/* If there are no faults, stop right there. If there are any add them to the list */
							if (!encounteredHighInp)
							{
								tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

								encounteredHighInp = true;
							}
							else
							{
								set_intersection(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end(),
									tempFaultList.begin(), tempFaultList.end(), std::inserter(intersectSet, intersectSet.begin()));

								/* Clear tempFault list and transfer to tempFaultList */
								tempFaultList.clear();

								tempFaultList = intersectSet;

								/* Clear intersect */
								intersectSet.clear();
							}


						}
						else if (g->node_inputs[i]->value == 0)
						{

							for (int x : idFaultMapping[g->node_inputs[i]->ID])
							{
								removeList.insert(x);
								tempFaultList.erase(x);
							}
						}

					}
					for (int x : removeList)
					{
						tempFaultList.erase(x);
					}

				}
				else
				{
					/* All faults causing values to controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{
						tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

					}
				}
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

			if (!inpNotFound)
			{
				/* Add fault lists */
				if (output == 1)
				{
					set<int> removeList;
					set<int> intersectSet;
					bool encounteredLowInp = false;
					/* All faults causing values to non-controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{

						if (g->node_inputs[i]->value == 0)
						{
							if (!encounteredLowInp)
							{
								//tempFaultList.insert(idFaultMapping[nodeList[i].ID].begin(), idFaultMapping[nodeList[i].ID].end());
								tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

								encounteredLowInp = true;
							}
							else
							{
								set_intersection(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end(),
									tempFaultList.begin(), tempFaultList.end(), std::inserter(intersectSet, intersectSet.begin()));

								/* Clear tempFault list and transfer to tempFaultList */
								tempFaultList.clear();

								tempFaultList = intersectSet;

								/* Clear intersect */
								intersectSet.clear();
							}

						}
						else if (g->node_inputs[i]->value == 1)
						{
							for (int x : idFaultMapping[g->node_inputs[i]->ID])
							{
								removeList.insert(x);
								tempFaultList.erase(x);
							}
						}

					}
					for (int x : removeList)
					{
						tempFaultList.erase(x);
					}

				}
				else
				{
					/* All faults causing values to controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{
						tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

					}
				}
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

			if (!inpNotFound)
			{
				/* Add fault lists */
				if (output == 0)
				{
					set<int> removeList;
					set<int> intersectSet;
					bool encounteredHighInp = false;
					/* All faults causing values to non-controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{

						if (g->node_inputs[i]->value == 1)
						{
							if (!encounteredHighInp)
							{
								//tempFaultList.insert(idFaultMapping[nodeList[i].ID].begin(), idFaultMapping[nodeList[i].ID].end());
								tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());
								encounteredHighInp = true;
							}
							else
							{
								set_intersection(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end(),
									tempFaultList.begin(), tempFaultList.end(), std::inserter(intersectSet, intersectSet.begin()));

								/* Clear tempFault list and transfer to tempFaultList */
								tempFaultList.clear();

								tempFaultList = intersectSet;

								/* Clear intersect */
								intersectSet.clear();
							}

						}
						else if (g->node_inputs[i]->value == 0)
						{

							for (int x : idFaultMapping[g->node_inputs[i]->ID])
							{
								removeList.insert(x);
								tempFaultList.erase(x);
							}
						}

					}
					for (int x : removeList)
					{
						tempFaultList.erase(x);
					}

				}
				else
				{
					/* All faults causing values to controlling values */
					for (int i = 0; i < g->node_inputs.size(); i++)
					{
						tempFaultList.insert(idFaultMapping[g->node_inputs[i]->ID].begin(), idFaultMapping[g->node_inputs[i]->ID].end());

					}
				}
			}

			break;

		case INV:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			output = !g->node_inputs[0]->value;

			if (!inpNotFound)
			{
				tempFaultList = idFaultMapping[g->node_inputs[0]->ID];
			}

			break;

		case BUF:
			if (g->node_inputs[0]->value == -1)
			{
				inpNotFound = true;
				break;
			}

			output = g->node_inputs[0]->value;

			if (!inpNotFound)
			{
				tempFaultList = idFaultMapping[g->node_inputs[0]->ID];

			}

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

			string temp = to_string(g->node_outputs[0]->ID) + " stuck at " + to_string(1 - output);
			auto it = find(allFaults.begin(), allFaults.end(), temp);

			auto index = distance(allFaults.begin(), it);

			tempFaultList.insert(index);

			idFaultMapping[g->node_outputs[0]->ID] = tempFaultList;
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
	for (auto g : gateList)
	{
		/* Just for the gates that have the faulty node as one of their inputs */
		if (find(g->node_inputs.begin(), g->node_inputs.end(), stuckAtFault) != g->node_inputs.end())
		{
			/* Add to the D frontier */
			dFrontier.push_back(g);
		}
	}
}

pair<int, int> objective()
{
	// Initially you just need to check if the node that we're talking about has its value set to the opp of the stuck at the value.
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
		}

	}

	return result;
}

void podem()
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

	stuckAtFault = 0; //For now we'll just look at the fault at the start of the fautl list

	// Populate the D frontier based on just the stuck at fault as that's the only plac ewe have a faulty input that is a D or a Dbar
	stuckAtValue = 1;

	/* Invoke the D setup setup method */
	dFrontierSetup();

	/* Invoke the test simulator */
	podem();


	return 0;
}