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



				//cout << "ID mapping " << idFaultMapping[ID].size() << endl;

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

	/* Invoke the test simulator */


	return 0;
}