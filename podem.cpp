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
int expectedTargetValue;

/* Define the D frontier in a list */
list<Gate*> dFrontier;

/* Implication stack
	If something doesn't work out pop the last immplication from the stack. 
	This should help checkpoint and revisit the last working state, once we wipe clean the node values after a failure .
	We need to maintain this stack only for the principal inputs as those are really what we care about. */
list<pair<int, int>> implicationStack;

bool implicationDone = false;

bool faultSiteActivated = false;

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
	cout << "Total unresolved gates: " << unresolvedGates.size() << endl;
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
				if (g->node_inputs[i]->analysed == false)
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
				if (g->node_inputs[i]->analysed == false)
				{
					inpNotFound = true;
					break;
				}

				//output |= g->node_inputs[i]->value;
				output = gateOutputLookupOr[g->node_inputs[i]->value][output];
			}

			break;

		case NAND:
			output = 1;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->analysed == false)
				{
					inpNotFound = true;
					break;
				}

				//output |= !(g->node_inputs[i]->value);
				//output = gateOutputLookupOr[gateOutputLookupInv[g->node_inputs[i]->value]][output];
				output = gateOutputLookupAnd[output][g->node_inputs[i]->value];
			}
			output = gateOutputLookupInv[output];

			break;

		case NOR:
			output = 0;
			for (int i = 0; i < g->node_inputs.size(); i++)
			{
				if (g->node_inputs[i]->analysed == false)
				{
					inpNotFound = true;
					break;
				}

				//output &= !g->node_inputs[i]->value;
				//output = gateOutputLookupAnd[gateOutputLookupInv[g->node_inputs[i]->value]][output];
				
				output = gateOutputLookupOr[g->node_inputs[i]->value][output];
			}
			output = gateOutputLookupInv[output];

			break;

		case INV:
			if (g->node_inputs[0]->analysed == false)
			{
				inpNotFound = true;
				break;
			}

			//output = !g->node_inputs[0]->value;
			output = gateOutputLookupInv[g->node_inputs[0]->value];

			break;

		case BUF:
			if (g->node_inputs[0]->analysed == false)
			{
				inpNotFound = true;
				break;
			}

			//output = g->node_inputs[0]->value;
			output = gateOutputLookupBuf[g->node_inputs[0]->value];

			break;

		}

		if (!inpNotFound)
		{
			if (g->node_outputs[0]->ID == stuckAtFault)
			{
				if (stuckAtValue == 0)
				{
					if (output == 1)
					{
						output = VAL_D;
					}
				}
				else
				{
					if (output == 0)
					{
						output = VAL_DBAR;
					}
				}
			}
			g->node_outputs[0]->value = output;
			g->resolved = true; //can be removed

			g->node_outputs[0]->analysed = true;

			//cout << "------- Implication---------" << endl;
			//cout << "output: " << output << endl;;

		}
	}



	auto final = remove_if(unresolvedGates.begin(), unresolvedGates.end(), doRemove);
	unresolvedGates.erase(final, unresolvedGates.end());

	allOutputsAvailable = !(unresolvedGates.size());

}

void resetNodes()
{
	for (auto node : nodeList)
	{
		node->value = VAL_X;
		node->analysed = false;
	}
}

void dFrontierRefresh()
{
	/* Update dFrontier */
	dFrontier.clear();

	for (auto g : gateList)
	{
		/* If the gate has at least one of its inputs as D or D bar and the output is still X */
		if (g->node_outputs[0]->value == VAL_X)
		{
			/* Check if one of the inputs is D */
			for (auto node : g->node_inputs)
			{
				if (node->value == VAL_D || node->value == VAL_DBAR)
				{
					/* Add to dFrontier */
					dFrontier.push_back(g);
					break;
				}
			}
		}
	}

}

void imply()
{
	/* Set all node values to X */
	resetNodes();

	/* Set input values based on implication stack */
	for (auto imp : implicationStack)
	{
		cout << "Implication: " << imp.first << " " << imp.second << endl;
		auto node = nodeIDMapping[imp.first];
		node->value = imp.second;
	}

	/* Set analyzed to true for input nodes */
	for (auto node : inpNodesList)
	{
		nodeIDMapping[node]->analysed = true;
	}

	/* Set all gates to unresolved */
	for (auto &g : gateList)
	{
		unresolvedGates.push_back(g); //TODO make sure this is right
		g->resolved = false;
	}
	allOutputsAvailable = false;

	while (!allOutputsAvailable)
	{
		processGateOutput();
	}

	dFrontierRefresh();

}

pair<int, int> objective()
{
	// Initially you just need to check if the faulty node has its value set to the opp of the stuck at the value.
	// If not, then we just need to returnn as objective the node idx and the vaue we want, which is the stuck at value;s complement
	pair<int, int> result;
	result.first = 1001;

	// Assume that the fault under consideration is g
	if (!faultSiteActivated)
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

				break;

			}
		}

	}

	return result;
}

pair<int, int> backtrace(pair<int, int> objec)
{

	/* if objec is already a PI, return
	   Else, try to work through one of the gates' inputs that's not already been assigned a value */
	//if (find(inpNodesList.begin(), inpNodesList.end(), objec.first) != inpNodesList.end())
	if (!nodeIDMapping[objec.first]->gate_output_of.empty())
	{
		//cout << "Entered here\n";
		cout << ((struct Gate*)nodeIDMapping[objec.first]->gate_output_of[0])->gType << " Gate type\n";
		/* Loop over the inputs and see if you are able to get a positive  */
		for (auto inp : ((Gate*)nodeIDMapping[objec.first]->gate_output_of[0])->node_inputs)
		{
			if (inp->value != VAL_X)
			{
				continue;
			}
			auto g = (Gate*)nodeIDMapping[objec.first]->gate_output_of[0];

			int invParity = (g->gType + 1) % 2 ; // TODO Honestly the order of the enum just worked out brilliantly like this

			pair<int, int> btRes = backtrace(pair<int, int>(inp->ID, objec.second ^ invParity));

			if (btRes.first != -1)
			{
				return btRes;
			}
		}
	}
	else
	{
		// If the stuck at fault is at a primary input
		if (objec.first == stuckAtFault)
		{
			objec.second = stuckAtValue ? VAL_DBAR : VAL_D;
		}
		
	}

	return objec;
}

int podem()
{
	static int podemCounter = 0;

	podemCounter++;
	/* Check error at PO */
	bool errorAtPO = false;
	for (auto n : outNodesList)
	{
		if ((nodeIDMapping[n]->value == VAL_D || nodeIDMapping[n]->value == VAL_DBAR) && implicationDone)
		{
			errorAtPO = true;
			return 0; //SUCCESS
			break;
		}
	}
	
	/* Check for inconsistency at fault site */
	if (implicationDone && (nodeIDMapping[stuckAtFault]->value <= VAL_ONE))
	{
		cout << "The faulty value at the node was " << nodeIDMapping[stuckAtFault]->value << endl;
		return -1; // FAILURE
	}
	else if (implicationDone && (nodeIDMapping[stuckAtFault]->value == expectedTargetValue))
	{
		/* Fault site is now activated */
		faultSiteActivated = true;
	}

	/* Check if D frontier is empty. If we've reached here it means there's no error at PO anyway */
	if (dFrontier.empty() && !implicationStack.empty())
	{
		cout << "The faulty value at the node was " << nodeIDMapping[stuckAtFault]->value << endl;

		if (faultSiteActivated)
		{
			return -1;
		}

		
	}

	/* Get an objective */
	pair<int, int> objec = objective();

	cout << "Objective: ";
	cout << objec.first << " " << objec.second << endl;


	/* Backtrace and get a PI */
	pair<int, int> pI = backtrace(objec);

	cout << "Backtraced PI: " << pI.first << " " << pI.second << endl;

	/* Imply PI - no need to check that'd be done once you call PODEM subsequently */
	/* Add the newest pi to the stack and imply */
	implicationStack.push_back(pI);

	imply(); // I think we need to do this in a way that the the outputs aren't inputs

	for (auto elem : nodeList)
	{
		cout << "Implied value of node " << elem->ID << " was " << elem->value << endl;
	}
	cout << "D frontier consists of: " << endl;
	for (auto elem : dFrontier)
	{
		cout << elem->gType << endl;
	}

	if (podemCounter == 6)
	{
		//return 0;
	}

	implicationDone = true;

	//return 0;

	if (!podem()) // SUCCESS
	{
		return 0;
	}

	cout << "This imply didn't work, lemme change\n";

	/* Reverse the implication. */
	implicationStack.pop_back();
	implicationStack.push_back(pair<int, int>(pI.first, 1 - pI.second));

	imply();

	

	if (!podem())
	{
		return 0;
	}
	else
	{
		/* Pop from stack and reset the nodes */
		implicationStack.pop_back();
		resetNodes(); // Reset nodes based on the implication stack

		return -1;
	}

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

		int gateOutNodeID;

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
				//unresolvedGates.push_back(newGate);
			}
			else if (readingGate)
			{
				int ID = stoi(token);

				/* Store in the variable gateOutNodeID */
				gateOutNodeID = ID;

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


			nodeIDMapping[gateOutNodeID]->gate_output_of.push_back((void*)gateList.back());
			//nodeList.back()->gate_output_of.push_back((void *)gateList.back());

			cout << "Node " << gateOutNodeID << " is output of gate type " << gateList.back()->gType << endl;

			readingGate = false;
		}

		readingInpNodes = false;
		readingOutNodes = false;
	}

	inpFile.close();

	// Prepare the loookup tables for all the gates
	prepareGateLookup();

	resetNodes();

	stuckAtFault = 16; //For now we'll just look at the fault at the start of the fautl list

	// Populate the D frontier based on just the stuck at fault as that's the only plac ewe have a faulty input that is a D or a Dbar
	stuckAtValue = 0;

	expectedTargetValue = stuckAtValue ? VAL_DBAR : VAL_D;

	/* Invoke the D setup setup method */
	dFrontierRefresh();

	/* Add to dFrontier before starting */
	cout << "Got to this point\n";
	for (auto g : nodeIDMapping[stuckAtFault]->gate_input_to)
	{
		//dFrontier.push_back((struct Gate*)g);
	}

	/* Invoke the test simulator */
	int ret = podem();

	if (ret)
	{
		cout << "Failed -----------------" << endl;
		return 0;

	}
	cout << "DONE  --------------------\n";
	/* Print out the implication stack */
	for (auto imp : implicationStack)
	{
		cout << imp.first << " " << imp.second << endl;
	}


	return 0;
}