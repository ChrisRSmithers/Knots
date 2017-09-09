#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <regex>
#include <utility>
#include <algorithm>
#include <time.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/ref.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/planar_canonical_ordering.hpp>
#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/algorithm/string.hpp>
#include <tuple>

#include <Windows.h>//Used for sleep during testing, REMOVE AT COMPLETION

using namespace std;

//Class Defs
//--------------------------------------------------------------------------------------
class EdgeDetail{
	public:
		int end1, end2, place1, place2, edgeNumber;
	};

class GraphStructure{
public:
	vector<vector<EdgeDetail>> WedgeList;
	vector<EdgeDetail> EdgeList;
};

class FaceList{
public:
	vector<vector<EdgeDetail>> Faces;
	vector<int> startVertex;
};

//--------------------------------------------------------------------------------------



//Converts a string to a more usable form
vector<vector<pair<int,int>>> preProcess(string gaussCode){
	vector<vector<pair<int,int>>> gaussParagraph;
	bool Flag = true;
	while (Flag){
		vector<pair<int, int>> NewWord;
		//Pick out a Gauss Word (each is separated by colons)
		size_t Found = gaussCode.find(':');
		string GaussWord;
		if (Found == string::npos){
			GaussWord = gaussCode;
			gaussCode.clear();
		}
		else{
			GaussWord = gaussCode.substr(0, Found);
			gaussCode.erase(0, Found + 1);
		}
		//Pick out each character from the alphabet A and place in a vector
		vector<string> strings;
		boost::split(strings, GaussWord, boost::is_any_of("\t "));
		for (auto iter = strings.begin(); iter != strings.end(); iter++) {
			string Temp = *iter;
			if (Temp.size() != 0){
				//Pick out the sign, and remove from the string if needbe
				char FirstPart = Temp[0];
				int sign = 0;
				if (FirstPart == '+'){
					sign = 1;
					Temp.erase(0, 1);
				}
				else if (FirstPart == '-'){
					sign = -1;
					Temp.erase(0, 1);
				}
				pair<int, int> newPair = make_pair(stoi(Temp), sign);
				NewWord.push_back(newPair);
				//Add this characters description to the word
			}
		}
		gaussParagraph.push_back(NewWord);
		//Add the word to the apragraph
		if (gaussCode.size() == 0){
			Flag = false;
			//Is the paragraph finished?
		}
	}
	//Check the Paragraph is as it should be:
	/*
	for (auto it1 = gaussParagraph.begin(); it1 != gaussParagraph.end(); it1++){
		auto Temp = *it1;
		for (auto it2 = Temp.begin(); it2 != Temp.end(); it2++){
			auto InnerTemp = *it2;
			std::cout << InnerTemp.first << "    " << InnerTemp.second << endl;
		}
		std::cout << endl << endl;
	}
	*/
	return gaussParagraph;
}

//Stage 1.1 of the algorithm, undoing Reidemeister I
void ReidemeisterRemove(vector<vector<pair<int,int>>> &gaussParagraph, int &vertexNum){
	cout << endl << "----------------------------------------------" << endl << "Stage 1.1 Starting" << endl;
	vector<int> Replaced;
	for (auto iter = gaussParagraph.begin(); iter != gaussParagraph.end(); iter++){
		auto CurrentWord = *iter;
		int counter = 0;
		bool flag = true;
		bool SameChar;
		while (flag){
			if (CurrentWord.size() < 2){
				std::cout << "Warning, too much reduction";

			}
			if (counter != CurrentWord.size() - 1){
				SameChar = (CurrentWord[counter].first == CurrentWord[counter + 1].first);
				if (SameChar){
					Replaced.push_back(CurrentWord[counter].first);
					CurrentWord.erase(CurrentWord.begin() + counter, CurrentWord.begin() + counter + 2);
					if (counter != 0){
						counter--;
					}
				}
				else {
					counter++;
				}
			}
			else {
				flag = false;
			}
		}
		flag = true;
		
		while (flag){
			if (CurrentWord[CurrentWord.size() - 1].first == CurrentWord[0].first){
				Replaced.push_back(CurrentWord[0].first);
				CurrentWord.erase(CurrentWord.begin());
				CurrentWord.erase(CurrentWord.end() - 1);
			}
			else {
				flag = false;
			}
		}
		*iter = CurrentWord;

	}
	vertexNum = 0;
	for (auto iter = gaussParagraph.begin(); iter != gaussParagraph.end(); iter++){
		auto word = *iter;
		vertexNum = vertexNum + word.size();
	}
	vertexNum = vertexNum / 2;
	//auto temporary = remove_if(Replaced.begin(), Replaced.end(), bind2nd(greater<int>(), vertexNum));
	//Replaced.erase(temporary, Replaced.end());
	
	for (auto iter = Replaced.begin(); iter != Replaced.end(); iter++){
		cout << endl;
		cout << "The crossing labelled by " << *iter << " was removed (Reidemeister I)" << endl;
	}

	

	for (size_t iter = 0; iter < gaussParagraph.size(); iter++){
		auto word = gaussParagraph[iter];
		for (size_t iter2 = 0; iter2 < word.size(); iter2++){
			auto temp = word[iter2];
			if (temp.first > vertexNum){
				int Place = temp.first;
				while (Place > vertexNum){
					Place = Replaced[Place - vertexNum - 1];
				}
				gaussParagraph[iter][iter2].first = Place;
			}
		}
	}

	cout << endl << "Stage 1.1 Complete" << endl << "----------------------------------------------";

	//Testing output code
	/*
	for (auto iter = gaussParagraph[1].begin(); iter != gaussParagraph[1].end(); iter++){
		auto temp = *iter;
		std::cout << temp.first << "  ";
	}
	*/
}

//Stage 1.2 of the algorithm, Building a graph
void AbstractGraph(vector<vector<pair<int, int>>> &gaussParagraph, vector<int> &nodeType, int &vertexNum, GraphStructure &ourGraph){
	cout << endl << "----------------------------------------------" << endl << "Stage 1.2 Starting" << endl;
	nodeType.resize(vertexNum + 1);
	for (auto iter = gaussParagraph.begin(); iter != gaussParagraph.end(); iter++){
		auto Word = *iter;
		for (auto iter2 = Word.begin(); iter2 != Word.end(); iter2++){
			auto Character = *iter2;
			if (Character.second != 0){
				nodeType[Character.first] = Character.second;
			}
		}
	}

	//Check nodeType is as desired
	/*
	std::cout << "nodeType =  ";
	for (auto iter = nodeType.begin(); iter != nodeType.end(); iter++){
		std::cout << *iter << "  ";
	}
	*/

	ourGraph.WedgeList.resize(vertexNum + 1);
	for (int iter = 1; iter < vertexNum + 1; iter++){
		ourGraph.WedgeList[iter].resize(4);
	}
	int edgeLabel = 0;
	for (auto wordNum = gaussParagraph.begin(); wordNum != gaussParagraph.end(); wordNum++){
		auto currentWord = *wordNum;
		for (size_t iter = 0; iter < currentWord.size(); iter++){
			int vertex1;
			int vertex2;
			int place1;
			int place2;
			if (iter + 1 < currentWord.size()){
				vertex1 = currentWord[iter].first;
				vertex2 = currentWord[iter + 1].first;
				
				if (currentWord[iter].second == 1){
					place1 = 3;
				}
				else if (currentWord[iter].second == -1){
					place1 = 1;
				}
				else{
					place1 = 2;
				}

				if (currentWord[iter + 1].second == 1){
					place2 = 1;
				}
				else if (currentWord[iter + 1].second == -1){
					place2 = 3;
				}
				else{
					place2 = 0;
				}
			}
			else {
				vertex1 = currentWord[iter].first;
				vertex2 = currentWord[0].first;
				if (currentWord[iter].second == 1){
					place1 = 3;
				}
				else if (currentWord[iter].second == -1){
					place1 = 1;
				}
				else{
					place1 = 2;
				}

				if (currentWord[0].second == 1){
					place2 = 1;
				}
				else if (currentWord[0].second == -1){
					place2 = 3;
				}
				else{
					place2 = 0;
				}
			}
			EdgeDetail ourEdge;
			ourEdge.place1 = place1;
			ourEdge.place2 = place2;
			ourEdge.end1 = vertex1;
			ourEdge.end2 = vertex2;
			ourEdge.edgeNumber = edgeLabel;
			ourGraph.WedgeList[vertex1][place1] = ourEdge;
			ourGraph.WedgeList[vertex2][place2] = ourEdge;
			ourGraph.EdgeList.push_back(ourEdge);
			edgeLabel++;
		}

	}

	cout << endl << "Stage 1.2 Complete" << endl << "----------------------------------------------";

	//Test Abstract Graph is as desired
	/*
	for (auto iter = ourGraph.EdgeList.begin(); iter != ourGraph.EdgeList.end(); iter++){
		auto Temp = *iter;
		cout << endl << Temp.end1 << "   " << Temp.end2;
	}
	cout << endl;
	for (auto iter = ourGraph.WedgeList.begin(); iter != ourGraph.WedgeList.end(); iter++){
		auto Temp = *iter;
		for (auto iter2 = Temp.begin(); iter2 != Temp.end(); iter2++){
			auto wedge = *iter2;
			cout << "Wedge = (" << wedge.end1 << ", " << wedge.end2 << "), " << wedge.place1 << ", " << wedge.place2 << ", " << wedge.edgeNumber << endl;
		}
	}
	*/
}

//Stage 1.3 of the algorithm, Removing multiple edges
void DeleteMultiple(GraphStructure &ourGraph, vector<int> &nodeType, int &vertexNum){
	cout << endl << "----------------------------------------------" << endl << "Stage 1.3 Starting" << endl;
	for (int iter = 1; iter < vertexNum +1; iter++){
		auto currentWedge = ourGraph.WedgeList[iter];
		bool flag = true;
			vector<int> neighbourList;
			for (auto iter2 = currentWedge.begin(); iter2 != currentWedge.end(); iter2++){
				auto temp = *iter2;
				if (temp.end1 != iter){
					neighbourList.push_back(temp.end1);
				}
				else {
					neighbourList.push_back(temp.end2);
				}
			}
			
			if (neighbourList.size() > 2){
				if (neighbourList[0] == neighbourList[1]){
					auto edgetochange = currentWedge[1];
					vertexNum++;
					cout << endl << "A vertex labelled   " << vertexNum << "  of degree 2 was added between nodes  "
						<< edgetochange.end1 << "  and  " << edgetochange.end2 << endl;
					
					if (edgetochange.end1 == iter){
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end2 = vertexNum;
						newEdge1.place2 = 0;
						newEdge2.end1 = vertexNum;
						newEdge2.place1 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;

					}
					else {
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end1 = vertexNum;
						newEdge1.place1 = 0;
						newEdge2.end2 = vertexNum;
						newEdge2.place2 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;
					}
				}
				if (neighbourList[1] == neighbourList[2]){
					auto edgetochange = currentWedge[2];
					vertexNum++;
					cout << endl << "A vertex labelled   " << vertexNum << "  of degree 2 was added between nodes  "
						<< edgetochange.end1 << "  and  " << edgetochange.end2 << endl;
					if (edgetochange.end1 == iter){
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end2 = vertexNum;
						newEdge1.place2 = 0;
						newEdge2.end1 = vertexNum;
						newEdge2.place1 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;
					}
					else {
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end1 = vertexNum;
						newEdge1.place1 = 0;
						newEdge2.end2 = vertexNum;
						newEdge2.place2 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;
					}
				}
				if (neighbourList[2] == neighbourList[3]){
					auto edgetochange = currentWedge[3];
					vertexNum++;
					cout << endl << "A vertex labelled   " << vertexNum << "  of degree 2 was added between nodes  " 
						<< edgetochange.end1 << "  and  " << edgetochange.end2 << endl;
					if (edgetochange.end1 == iter){
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end2 = vertexNum;
						newEdge1.place2 = 0;
						newEdge2.end1 = vertexNum;
						newEdge2.place1 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;
					}
					else {
						auto newEdge1 = edgetochange;
						auto newEdge2 = edgetochange;
						newEdge1.end1 = vertexNum;
						newEdge1.place1 = 0;
						newEdge2.end2 = vertexNum;
						newEdge2.place2 = 1;
						newEdge2.edgeNumber = ourGraph.EdgeList.size();

						ourGraph.EdgeList[newEdge1.edgeNumber] = newEdge1;
						ourGraph.EdgeList.push_back(newEdge2);

						vector<EdgeDetail> newWedge;
						newWedge.push_back(newEdge1);
						newWedge.push_back(newEdge2);
						ourGraph.WedgeList.push_back(newWedge);

						ourGraph.WedgeList[newEdge1.end1][newEdge1.place1] = newEdge1;
						ourGraph.WedgeList[newEdge1.end2][newEdge1.place2] = newEdge1;
						ourGraph.WedgeList[newEdge2.end1][newEdge2.place1] = newEdge2;
						ourGraph.WedgeList[newEdge2.end2][newEdge2.place2] = newEdge2;
					}
				}
			}
	}

	cout << endl << "Stage 1.3 Complete" << endl << "----------------------------------------------";

	//Test subdivided graph is as desired
	/*
	for (auto iter = ourGraph.EdgeList.begin(); iter != ourGraph.EdgeList.end(); iter++){
		auto Temp = *iter;
		cout << endl << Temp.end1 << "   " << Temp.end2;
	}
	cout << endl;
	for (auto iter = ourGraph.WedgeList.begin()+1; iter != ourGraph.WedgeList.end(); iter++){
		auto Temp = *iter;
		cout << "Wedge = " << endl;
		for (auto iter2 = Temp.begin(); iter2 != Temp.end(); iter2++){
			auto edge = *iter2;
			cout << "Edge = (" << edge.end1 << ", " << edge.end2 << "), " << edge.place1 << ", " << edge.place2 << ", " << edge.edgeNumber << endl;
		}
	}
	*/
}

//Stage 1.4 of the algorithm, restricting to connected components
void connectedComp(GraphStructure &ourGraph, vector<int> &nodeType, int &vertexNum){
	cout << endl << "----------------------------------------------" << endl << "Stage 1.4 Starting" << endl;
	cout << endl << "Stage 1.4 Complete" << endl << "----------------------------------------------";
}

//Stage 1.5 of the algorithm, Finding cycles
void findCycles(GraphStructure &ourGraph, vector<int> &nodeType, int &vertexNum, FaceList &FaceList){
	cout << endl << "----------------------------------------------" << endl << "Stage 1.5 Starting" << endl;
	FaceList.Faces.resize(0);
	int edgeNumber = ourGraph.EdgeList.size();
	vector<bool> edgeBools(2*edgeNumber, false);
	int counter = 0;
	bool flag = true;
	while (flag){

		vector<EdgeDetail> NewFace;
		EdgeDetail currentEdge;
		EdgeDetail nextEdge;
		EdgeDetail firstEdge;
		int startVertex;

		while (edgeBools[counter]){
			counter++;
			if (counter == edgeBools.size()){
				flag = false;
				counter = 0;
				edgeBools[0] = false;
			}
		}
		if (flag){
			edgeBools[counter] = true;
		}
		bool CurrentDirection;//true indicates forward, flase indicates backwards
		bool nextDirection;
		if (flag){
			if (counter > edgeNumber - 1){
				currentEdge = ourGraph.EdgeList[counter - edgeNumber];
				CurrentDirection = false;
				startVertex = currentEdge.end2;
			}
			else {
				currentEdge = ourGraph.EdgeList[counter];
				CurrentDirection = true;
				startVertex = currentEdge.end1;
			}
			firstEdge = currentEdge;
			NewFace.push_back(currentEdge);
			bool innerflag = true;
			while (innerflag){
				if (CurrentDirection){
					if (currentEdge.place2 == ourGraph.WedgeList[currentEdge.end2].size() - 1){
						nextEdge = ourGraph.WedgeList[currentEdge.end2][0];

					}
					else {
						nextEdge = ourGraph.WedgeList[currentEdge.end2][currentEdge.place2 + 1];
					}
					nextDirection = (nextEdge.end1 == currentEdge.end2);
				}
				else{
					if (currentEdge.place1 == ourGraph.WedgeList[currentEdge.end1].size() - 1){
						nextEdge = ourGraph.WedgeList[currentEdge.end1][0];
					}
					else {
						nextEdge = ourGraph.WedgeList[currentEdge.end1][currentEdge.place1 + 1];
					}
					nextDirection = (nextEdge.end1 == currentEdge.end1);
				}
				NewFace.push_back(nextEdge);
				currentEdge = nextEdge;
				CurrentDirection = nextDirection;
				if (currentEdge.edgeNumber == firstEdge.edgeNumber){
					innerflag = false;
				}
				if (CurrentDirection){
					edgeBools[currentEdge.edgeNumber] = true;
				}
				else{
					edgeBools[currentEdge.edgeNumber + edgeNumber] = true;
				}
			}
			FaceList.Faces.push_back(NewFace);
			FaceList.startVertex.push_back(startVertex);
		}

	}

	cout << endl <<"The cycles are given by the following ordered lists of nodes"<< endl;

	for (unsigned int iter = 0; iter < FaceList.Faces.size(); iter++){
		auto currentFace = FaceList.Faces[iter];
		int startVertex = FaceList.startVertex[iter];
		
		vector<int> currentCycle;
		currentCycle.push_back(startVertex);
		cout << endl << "Cycle Number " << iter + 1 << " is given by:" << endl;
		for (auto iter2 = currentFace.begin(); iter2 != currentFace.end() - 1; iter2++){
			EdgeDetail currentEdge = *iter2;
			int last = currentCycle[currentCycle.size() - 1];
			if (currentEdge.end1 == last){
				currentCycle.push_back(currentEdge.end2);
			}
			else {
				currentCycle.push_back(currentEdge.end1);
			}
			
		}
		for (auto it2 = currentCycle.begin(); it2 != currentCycle.end(); it2++){
			cout << *it2 << "  ";
		}
		cout << endl;
	}

	cout << endl << "Stage 1.5 Complete" << endl << "----------------------------------------------";

}

//Stage 2.1 of the algorithm, Maximal Planar Graph
void makeMaximal(GraphStructure &ourGraph, GraphStructure &maxGraph, vector<int> &nodeType, int &vertexNum, FaceList &FaceList){
	vector<vector<int>> cycleList;
	maxGraph = ourGraph;
	for (size_t iter = 0; iter < FaceList.Faces.size(); iter++){
		auto currentFace = FaceList.Faces[iter];
		int startVertex = FaceList.startVertex[iter];
		vector<int> currentCycle;
		currentCycle.push_back(startVertex);
		for (auto iter2 = currentFace.begin(); iter2 != currentFace.end() - 1; iter2++){
			auto currentEdge = *iter2;
			int last = currentCycle[currentCycle.size() - 1];
			if (currentEdge.end1 == last){
				currentCycle.push_back(currentEdge.end2);
			}
			else {
				currentCycle.push_back(currentEdge.end1);
			}
		}
		cycleList.push_back(currentCycle);
	}

	size_t cycleNumber = cycleList.size();
	for (size_t iter = 0; iter < cycleNumber; iter++){
		vector<int> cycleToSplit = cycleList[iter];
		while (cycleToSplit.size()>3){
			auto tempWedge = ourGraph.WedgeList[cycleToSplit[0]];
			vector<int> neighbourList;
			for (auto it2 = tempWedge.begin(); it2 != tempWedge.end(); it2++){
				auto tempEdge = *it2;
				if (tempEdge.end1 == cycleToSplit[0]){
					neighbourList.push_back(tempEdge.end2);
				}
				else{
					neighbourList.push_back(tempEdge.end1);
				}
			}
			auto place = find(neighbourList.begin(), neighbourList.end(), cycleToSplit[2]);
			if (place != neighbourList.end()){

			}
		}
	}
}

int main() {

	

	//Initialisations
	string gaussCode;
	int vertexNum = 0;
	vector<vector<pair<int, int>>> gaussParagraph;
	vector<int> nodeType;
	GraphStructure ourGraph;
	FaceList Faces;


	//Take User inputs
	
	std::cout << "Please enter a Gauss Code, or type List for a selection of in built codes." << endl << "For advice on the form of the code, Type Help" << endl;
	
	getline(cin, gaussCode);
	if (gaussCode == "List"){
		std::cout << "Please type the appropriate number to select the code:" << endl << endl
			<< "1: (3_1)   Trefoil" << endl
			<< "2: (4_1)" << endl
			<< "3: (5_1)" << endl
			<< "4: (6_3)" << endl
			<< "5: Linked Trefoils (same orientation)" << endl
			<< "6: Star of David" << endl
			<< endl;
		string selection;
		getline(cin, selection);
		std::cout << endl;
		if (selection[0] == '1'){
			gaussCode = "1 +2 3 +1 2 +3";
		}
		else if (selection[0] == '2'){
			gaussCode = "1 +2 3 -4 2 +1 4 -3";
		}
		else if (selection[0] == '3'){
			gaussCode = "1 +2 3 +4 5 +1 2 +3 4 +5";
		}
		else if (selection[0] == '4'){
			gaussCode = "1 +6 2 +1 4 -5 6 +2 3 -4 5 -3";
		}
		else if (selection[0] == '5'){
			gaussCode = "1 +2 +4 5 3 +1 2 +3 : 4 +5 8 +7 6 +8 7 +6";
		}
		else if (selection[0] == '6'){
			gaussCode = "1 +2 3 +4 5 +6 : 2 +3 4 +5 6 +1";
		}
		else{
			exit(EXIT_FAILURE);
		}
	}
	if (gaussCode[0] == 'H'){
		std::cout << "Help goes here" << endl;
		exit(EXIT_FAILURE);
	}

	cout << endl << endl << "Your Gauss code is :   " << gaussCode << endl << endl << endl;


	gaussParagraph = preProcess(gaussCode);

	ReidemeisterRemove(gaussParagraph, vertexNum);

	AbstractGraph(gaussParagraph,nodeType, vertexNum, ourGraph);

	DeleteMultiple(ourGraph, nodeType, vertexNum);

	connectedComp(ourGraph, nodeType, vertexNum);

	findCycles(ourGraph, nodeType, vertexNum, Faces);

	return 0;
}