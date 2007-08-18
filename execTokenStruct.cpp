#include <sstream>
#include <stdlib.h>
#include <iostream>

#include "execTokenStruct.h"
#include "methodStorage.h"
#include "gjye++.h"
#include "miscTools.h"
#include "blockWrappers.h"


namespace exec {

//################ EXCHANGE TOKEN FOR DATA ################//
bool parseTokID(std::string tokID, int retArry[]) { // turn �#|#� into array of # and #
	if (tokID.find('�') != std::string::npos && tokID.find('�') != std::string::npos && tokID.find('|') != std::string::npos) {
		int sIndex = (tokID.find('�'));
		int eIndex = (tokID.find('�'));
		tokID = tokID.substr(sIndex+1,eIndex-sIndex);
		
		int index = (tokID.find('|'));
		retArry[0] = (int) tools::stringToInt(tokID.substr(0,index));
		retArry[1] = (int) tools::stringToInt(  tokID.substr(index+1,tokID.length())  );
		return true;
	} else {return false;}
}
/// ################################ ///





//################ EXECUTE VECTOR STRUCT ################//
std::string runVectorStruct(EnviroWrap * environment, TokenGroup * tGroup, std::string catalyst) { // builds temp vectors out of vector declarations ( [2,22,222,2222] )
	if (catalyst == "-1") {catalyst = tGroup->catalyst;}
	std::string catalystCpy;
	catalystCpy = catalyst;
	
	if (catalystCpy.at(0) == '[' && catalystCpy.at(catalystCpy.length()-1) == ']') {
		catalystCpy.replace(0,1,"");
		catalystCpy.replace(catalystCpy.length()-1,1,"");
		
		std::string tokenQualifier = environment->dataStructure.variableReferencer("_VECTOR_");
		
		environment->dataStructure.addVector(tokenQualifier, *(new VariableStorage), -1, true); // create an empty set 
		
		while (catalystCpy.length() > 0) { // loop through each element of the vector dec. 
			int delimiter = (catalystCpy.find(',') != std::string::npos ? catalystCpy.find(',') : catalystCpy.length());
			std::string headerName = "", tokenData = "", token = catalystCpy.substr(0,delimiter);
			
			if (token.find('�') != std::string::npos && token.find('�',token.find('�')) != std::string::npos) { // take care of sublevels 
				std::string tokID, tokVal;
				int sIndex = 0, eIndex = 0, tokArry[2];
				
				sIndex = (token.find('�'));
				eIndex = (token.find('�',sIndex));
				tokID = token.substr(sIndex,eIndex-sIndex+1);
				
				if (parseTokID(tokID,tokArry) == false) {break;} // stop parsing the vector (we couldn't successfully parse an internal token) 
				tokVal = tGroup->getData(tokArry[0],tokArry[1]);
				
				if (tokVal.at(0) == '[' && tokVal.at(tokVal.length()-1) == ']') { // a vector's inside of this vector 
					token = runVectorStruct(environment,tGroup,tokVal); // recurse
				} else if (tokVal.find('=',1) == std::string::npos || tokVal.at(0) == '(' && tokVal.at(tokVal.length()-1) == ')') { // don't catch non-parenthetical assignations! (they will be tree/hash headers) 
					token = runTokenStruct(environment,tGroup,token); // callback, give them the ENTIRE token string!  A QUICK FIX, used to just give them tokID!!! 
				} else {token = tokVal;}
			}
			
			// at this point, any assignations inside of a vector will be treated as tree/hash heads!! 
			
			
			//cout << "\ntoken runVectorStruct: " << token; // TMP

			if (token.find('=',1) != std::string::npos) { // vector declaration (single) 
				int oPar = token.find('=',1);
				int cPar = token.length();
				headerName = token.substr(0,oPar); // not including = 
				tokenData = token.substr(oPar+1,cPar-oPar-1);
				
				if (headerName.find('�') != std::string::npos) {headerName = runTokenStruct(environment,tGroup,headerName);} // call backs 
				if (tokenData.find('�') != std::string::npos) {tokenData = runTokenStruct(environment,tGroup,tokenData);}
				
			} else {tokenData = token;} // array declaration (single) 
			
			headerName = tools::prepareVectorData(&environment->dataStructure, headerName); // give us the values (take out of variable form) 
			
			//cout << "\nname/data runVectorStruct: " << headerName << " is " << tokenData <<endl; // TMP

			if (tokenData.at(0) == '%') { // add a vector to this vector (do we need to do a type check?) 
				environment->dataStructure.getVector(tokenQualifier)->addVector(    headerName, *environment->dataStructure.getVector(  tokenData.substr(1,tokenData.length() - 1)  )    );
			}
			else { // add a variable to this vector 
				tokenData = tools::prepareVectorData(&environment->dataStructure, tokenData); // get the _actual_ value (replace vars, etc.) 
				environment->dataStructure.getVector(tokenQualifier)->addVariable(headerName, tokenData);
			}
			
			catalystCpy.replace(0,delimiter+1,"");
		}
		
		tokenQualifier.insert(0,"%");
		return tokenQualifier;
	} else {return "%%";} // ut-oh 
}
/// ################################ ///





//################ EXECUTE TOKEN STRUCT ################//
FuncObj * instantSTDL(std::string * objType, std::string * postfixFuncData, EnviroWrap * environment) { // factory class method 
	if (environment == NULL) environment = new EnviroWrap();
	
	if (postfixFuncData != NULL && objType->at(objType->length()-1) == '=') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Assignation_Obj(&environment->dataStructure);
	}
	else if (postfixFuncData != NULL && objType->at(objType->length()-1) == '*') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Multiplication_Obj(&environment->dataStructure);
	}
	else if (postfixFuncData != NULL && objType->at(objType->length()-1) == '-') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Subtraction_Obj(&environment->dataStructure);
	}
	else if (postfixFuncData != NULL && objType->at(objType->length()-1) == '/') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Division_Obj(&environment->dataStructure);
	}
	else if (postfixFuncData != NULL && objType->at(objType->length()-1) == '%') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Modulus_Obj(&environment->dataStructure);
	}
	else if (postfixFuncData != NULL && objType->at(objType->length()-1) == '+') {
		*postfixFuncData = *objType;
		*objType = "right";
		return new Addition_Obj(&environment->dataStructure);
	}
	else if (*objType == "Add" || *objType == "Prefix" || *objType == "Postfix" || *objType == "Insert" || *objType == "Prepend" || *objType == "Append") return new Add_Obj(&environment->dataStructure);
	else if (*objType == "CharAt") return new CharAt_Obj(&environment->dataStructure);
	else if (*objType == "Exit" || *objType == "Die" || *objType == "Quit") return new Exit_Obj(&environment->dataStructure);
	else if (*objType == "IndexOf" || *objType == "LastIndexOf") return new IndexOf_Obj(&environment->dataStructure);
	else if (*objType == "Join") return new Join_Obj(&environment->dataStructure);
	else if (*objType == "Local" || *objType == "My") return new Local_Obj(&environment->dataStructure);
	else if (*objType == "Lc" || *objType == "Uc" || *objType == "Lower" || *objType == "Upper" || *objType == "Lowercase" || *objType == "Uppercase") return new LowerUpperCase_Obj(&environment->dataStructure);
	else if (*objType == "LcFirst" || *objType == "UcFirst") return new LowerUpperCaseFirst_Obj(&environment->dataStructure);
	else if (*objType == "Length" || *objType == "LengthOf") return new Length_Obj(&environment->dataStructure);
	else if (*objType == "Print") return new Print_Obj(&environment->dataStructure);
	else if (*objType == "Random" || *objType == "Rand") return new Random_Obj(&environment->dataStructure);
	else if (*objType == "Remove" || *objType == "Delete") return new Remove_Obj(&environment->dataStructure);
	else if (*objType == "Reverse") return new Reverse_Obj(&environment->dataStructure);
	else if (*objType == "Round") return new Round_Obj(&environment->dataStructure);
	else if (*objType == "Select") return new Select_Obj(&environment->dataStructure);
	else if (*objType == "Split") return new Split_Obj(&environment->dataStructure);
	else if (*objType == "ValueOf") return new ValueOf_Obj(&environment->dataStructure);
	else if (*objType == "contains" || *objType == "lacks") return new Contains_Obj(&environment->dataStructure);
	else if (*objType == "beginsWith" || *objType == "endsWith") return new beginsWith_Obj(&environment->dataStructure);
	else if (*objType == "isDefined" || *objType == "exists") return new isDefined_Obj(&environment->dataStructure);
	else return new Void_Obj(&environment->dataStructure);
}


std::string runTokenStruct(EnviroWrap * environment, TokenGroup * tGroup, std::string catalyst) { // driver ... internal tracking is of the form    ~ ^ ^ 
	if (catalyst == "-1") {catalyst = tGroup->catalyst;}
	std::string catalystCpy = ""; catalystCpy = catalyst;
	FuncObj * thisObj = NULL;
	
	std::vector<std::string> tVecVocab, tVecParams, tVecTypes;
	
	while (catalystCpy.find('�') != std::string::npos && catalystCpy.find('�',catalystCpy.find('�')) != std::string::npos) {
		bool isOperator = false;
		std::string tokID = "", levelType = "ValueOf", levelData = "", postfixFuncData = "";
		int sIndex = 0, eIndex = 0, tokArry[2];
		
		sIndex = (catalystCpy.find('�'));
		if (sIndex != std::string::npos) 
			eIndex = (catalystCpy.find('�',sIndex));
		if (eIndex == std::string::npos) {std::cout << "CRITERROR :: Malformation: Token mismatch " <<std::endl;exit(1);}
		tokID = catalystCpy.substr(sIndex,eIndex-sIndex+1);
		catalystCpy.replace(sIndex,eIndex-sIndex+1,"^");
		
		if (parseTokID(tokID,tokArry) == false) {std::cout << "ERROR :: Token miss " <<std::endl;break;} // token miss! 
		tokID = tGroup->getData(tokArry[0],tokArry[1]);
		
		
		if (tokID.at(0) == '{' && tokID.at(tokID.length()-1) == '}') { // catch generic block declarations 
			std::string blockData = tokID.substr(1,tokID.length()-2); // look ma, no braces! 
			BlockWrap * tempBlock = new BlockWrap(blockData, environment);
			if (SHOW_DEBUGGING) std::cout << "\n=-=-=-=-=-=-=-=-=-= ENTERING BLOCK =-=-=-=-=-=-=-=-=-=\n  contents: >>   " << blockData << "<<" <<std::endl;
			tempBlock->executeCode();
			catalystCpy.replace(catalystCpy.find('^'),1,"");
			delete tempBlock;
			continue;
		}
		else if (tokID.at(0) == '(' && tokID.at(tokID.length()-1) == ')') { // catch parens  ... OLD: tokID.find('(') != std::string::npos && tokID.find(')',tokID.find('(')) != std::string::npos
			levelType = "ValueOf";
			levelData = tokID.substr(1,tokID.length()-2);
		}
		else if (tokID.at(0) == '[' && tokID.at(tokID.length()-1) == ']') { // catch vector declarations 
			catalystCpy.replace(catalystCpy.find('^'),1,runVectorStruct(environment,tGroup,tokID));
			continue; // let runVectorStruct take care of everything thereafter (it can call on us if it wants) 
		}
		else if (tokID.at(0) == '"' && tokID.at(tokID.length()-1) == '"' || tokID.at(0) == '\'' && tokID.at(tokID.length()-1) == '\'') { // catch strings! (cleanEscapes) 
			std::string tokenQualifier = environment->dataStructure.variableReferencer("_STRING_");
			tokID = tools::prepareVectorData(&environment->dataStructure, tokID); // auto cleanEscapes
			if (SHOW_DEBUGGING) std::cout << "STRING :: " << tokID << " into " << tokenQualifier <<std::endl;
			environment->dataStructure.addVariable(tokenQualifier,tokID, -1, true);
			tokenQualifier.insert(0,"$");
			catalystCpy.replace(catalystCpy.find('^'),1,tokenQualifier);
			continue;
		}
		else if (tokID.at(0) != '�' && isalnum(tokID.at(0)) > 0) { // catch functions -- select�#|#� select �#|#� or select $/%  (� is considered alnum) 
			int oPar,cPar;
			oPar = tokID.find_first_not_of(letterChars);
				if (oPar == std::string::npos) oPar = tokID.length(); // no parameters 
			cPar = tokID.length();
			levelType = tokID.substr(0,oPar);
			if (oPar < tokID.length() && tokID.at(oPar) == ' ') {++oPar;} // don't include a leading space 
			
			if (oPar < cPar) levelData = tokID.substr(oPar,cPar-oPar);
			else levelData = ""; // no parameters 
		}
		else if (tokID.at(0) == '-' || tokID.at(0) == '+' || tokID.at(0) == '!') { // handle the unaries, pre-mods and logic. neg. separately 
			int oPar,cPar;
			oPar = tokID.find_first_of("-+!");
			cPar = tokID.length();
			levelType = tokID.substr(0,oPar+1);
			levelData = tokID.substr(oPar+1,cPar-oPar-1);
		}
		else if (tokID.find_first_of(operators,1) != std::string::npos) { // get the operators (after position 0) 
			int oPar,cPar;
			oPar = tokID.find_first_of(operators,1);
			cPar = tokID.length();
			levelType = tokID.substr(0,oPar+1);
			levelData = tokID.substr(oPar+1,cPar-oPar-1);
		}
		else {
			levelType = "ValueOf";
			levelData = tokID;
		}
		
		
		
		// ACTUALLY DO THE OBJECT CREATION (SET UP) HERE //
		if (thisObj == NULL && tVecVocab.size() == 0) {
			
			
			// IF HANDLER //
			if ( levelType == "If" || levelType == "ElseIf" ) {
				if (SHOW_DEBUGGING) std::cout << "IF BLOCK: " << levelData <<std::endl;
				
				if ( levelType == "ElseIf" && tGroup->insideIfBlock == false ) {std::cout << "CRITERROR :: Malformation: ElseIf expression not contained within If segment " <<std::endl;exit(1);}
				if ( levelType == "ElseIf" && tGroup->openIfBlock == false ) continue; // already handled, ignore 
				
				
				// GRAB EXPRESSION //
				sIndex = (catalystCpy.find('^'));
				catalystCpy.replace(sIndex,1,"~");
				
				std::string tokIDSub = "", ifReturnValue = "";
				int sIndexSub = 0, eIndexSub = 0;
				
				sIndexSub = (levelData.find('�'));
				if (sIndexSub != std::string::npos) 
					eIndexSub = (levelData.find('�',sIndexSub));
				if (eIndexSub == std::string::npos) {std::cout << "CRITERROR :: Malformation: Token mismatch " <<std::endl;exit(1);}
				tokIDSub = levelData.substr(sIndexSub,eIndexSub-sIndexSub+1);
				levelData.replace(sIndexSub,eIndexSub-sIndexSub+1,"^");
				
				ifReturnValue = tools::prepareVectorData( &environment->dataStructure, runTokenStruct(environment,tGroup,tokIDSub) ); // retrieve expression value 
				// END GRAB EXPRESSION // 
				
				
				tGroup->insideIfBlock = true; // automatic entrance 
				
				
				if (SHOW_DEBUGGING) std::cout << "IN (" << tGroup->insideIfBlock << ") : " << ifReturnValue <<std::endl;
				
				// HANDLE EXPR // 
				if ( !tools::isInteger( ifReturnValue ) ) {std::cout << "CRITERROR :: Malformation: If expression " <<std::endl;exit(1);}
				if ( (int) tools::stringToInt( ifReturnValue ) != 0 ) { // true expression 
					sIndexSub = (levelData.find('�'));
					if (sIndexSub != std::string::npos) 
						eIndexSub = (levelData.find('�',sIndexSub));
					if (eIndexSub == std::string::npos) {std::cout << "CRITERROR :: Malformation: Token mismatch " <<std::endl;exit(1);}
					tokIDSub = levelData.substr(sIndexSub,eIndexSub-sIndexSub+1);
					levelData.replace(sIndexSub,eIndexSub-sIndexSub+1,"^");
					
					runTokenStruct(environment,tGroup,tokIDSub); // execute If/ElseIf block 
					
					tGroup->openIfBlock = false;
				}
				else { // false expression 
					tGroup->openIfBlock = true;
				}
				// END HANDLE EXPR // 
				
				continue;
			}
			else if ( levelType == "Else" ) {
				if ( tGroup->insideIfBlock == false ) {std::cout << "CRITERROR :: Malformation: Else expression not contained within If segment " <<std::endl;exit(1);}
				if ( tGroup->openIfBlock == false ) continue;
				
				std::string tokIDSub = "";
				int sIndexSub = 0, eIndexSub = 0;
				
				sIndexSub = (levelData.find('�'));
				if (sIndexSub != std::string::npos) 
					eIndexSub = (levelData.find('�',sIndexSub));
				if (eIndexSub == std::string::npos) {std::cout << "CRITERROR :: Malformation: Token mismatch " <<std::endl;exit(1);}
				tokIDSub = levelData.substr(sIndexSub,eIndexSub-sIndexSub+1);
				levelData.replace(sIndexSub,eIndexSub-sIndexSub+1,"^");
				
				runTokenStruct(environment,tGroup,tokIDSub); // execute Else block 
				
				tGroup->insideIfBlock = false;
				tGroup->openIfBlock = false;
				continue;
			}
			else if ( tGroup->insideIfBlock == true ) {
				tGroup->insideIfBlock = false; // implicit close 
				tGroup->openIfBlock = false;
			}
			// END IF HANDLING //
			
			
			
			if (levelType == "Local" || levelType == "My" || levelType == "ValueOf" || levelType.at(levelType.length()-1) == '=' 
				|| levelType.at(levelType.length()-1) == '*' || levelType.at(levelType.length()-1) == '-'
				|| levelType.at(levelType.length()-1) == '/' || levelType.at(levelType.length()-1) == '%' || levelType.at(levelType.length()-1) == '+') {
					thisObj = instantSTDL(&levelType, &postfixFuncData, environment); // create it immediately (don't bother with matching) 
					isOperator = true;
			}
			
			sIndex = (catalystCpy.find('^'));
			catalystCpy.replace(sIndex,1,"~");
			
			 // grab prefixed result and store in postfixFuncData for parsing later ... remove prefixed data
			if (isOperator == false && environment->methodStructure.isPostFixFunc(levelType) == true) {
				int sIndex = (catalystCpy.rfind('%',catalystCpy.find('~'))), eIndex = catalystCpy.find('~'); // go for the gusto 
				if (sIndex == std::string::npos) sIndex = (catalystCpy.rfind('$',catalystCpy.find('~'))); // or fall back 
				if (sIndex == std::string::npos) {std::cout << "CRITERROR :: Malformation: Failure to grab prefix expression before " << levelType << "()!" <<std::endl;exit(1);}
				
				if (catalystCpy.at(eIndex-1) == ' ') --eIndex; // fix the extra space problem (not vector) 
				postfixFuncData = catalystCpy.substr(sIndex,eIndex-sIndex);
				if (eIndex < catalystCpy.length()-1 && catalystCpy.at(eIndex) == ' ') ++eIndex; // and then put it back 
				catalystCpy.replace(sIndex,eIndex-sIndex,"");
				if (SHOW_DEBUGGING) std::cout << "postfixFuncData: " << postfixFuncData <<std::endl; //TMP
			}
			
		}
		//// 
		
		
		
		// in this order, recursive execution // 
		if (postfixFuncData.find('�') != std::string::npos) {
			postfixFuncData = runTokenStruct(environment,tGroup,postfixFuncData); // we can't execute the object until everything above it is executed
		}
		
		if (levelType.find('�') != std::string::npos) { // is this a problem? (we used it above)
			levelType = runTokenStruct(environment,tGroup,levelType); // we can't execute the object until everything above it is executed
		}
		
		if (levelData.find('�') != std::string::npos) {
			levelData = runTokenStruct(environment,tGroup,levelData); // we can't execute the object until everything above it is executed
		}
		/// 
		
		
		// build our types vector for this params entry // 
		if (isOperator == false) {
			tVecVocab.push_back(levelType);
			tVecParams.push_back(levelData);
			
			if (levelData != "") {
				std::string varData = levelData.substr(1,levelData.length()-1);
				if (levelData.at(0) == '%') {
					VariableStorage * tempVector;
					tempVector = environment->dataStructure.vecStringToVector(&varData);
					varData = tools::prepareVectorData(&environment->dataStructure, varData);
					tVecTypes.push_back(tempVector->typeString(varData));
				} else {
					tVecTypes.push_back(environment->dataStructure.typeString(varData));
				}
			} else tVecTypes.push_back("");
			
			
			if (postfixFuncData != "") {
				tVecVocab.push_back("left");
				tVecParams.push_back(postfixFuncData);
				
				std::string varData = postfixFuncData.substr(1,postfixFuncData.length()-1);
				if (postfixFuncData.at(0) == '%') {
					VariableStorage * tempVector;
					tempVector = environment->dataStructure.vecStringToVector(&varData);
					varData = tools::prepareVectorData(&environment->dataStructure, varData);
					std::string type = tempVector->typeString(varData);
					if (type == "-1") type = "%"; // to fix isDefined 
					tVecTypes.push_back(type);
				} else {
					std::string type = environment->dataStructure.typeString(varData);
					if (type == "-1") type = "$"; // to fix isDefined 
					tVecTypes.push_back(type);
				}
			}
		}
		/// 
		
		
		if (thisObj != NULL) {
			if (thisObj->setLevelData(levelType,levelData)) {
				if (SHOW_DEBUGGING) std::cout << "PUSHSUC: " << levelType << "() " << thisObj->getLevelData(levelType) <<std::endl; //TMP
				
				if (postfixFuncData != "") { // and push postfixed function 
					if (thisObj->setLevelData("left",postfixFuncData)) {
						if (SHOW_DEBUGGING) std::cout << "PUSHSUC (post): " << thisObj->getLevelData("left") <<std::endl; //TMP
					} else {std::cout << "CRITERROR :: Malformation: Failure to push post function " << levelType << "()!" <<std::endl;exit(1);} //TMP
				}
				
			} else {std::cout << "CRITERROR :: Malformation: Failure to push function " << levelType << "()!" <<std::endl;exit(1);} //TMP
		}
		
		if (thisObj != NULL && levelType == "ValueOf") { // for valueof, purge the object after one loop  (fixes   Select (1),(3)  problem -- we can't chain the valueof method!) 
			// clean up //
			int sIndex=0,eIndex=0;
			sIndex = (catalystCpy.find('~'));
			eIndex = (catalystCpy.rfind('^',catalystCpy.length()-1));
			if (sIndex > eIndex) {eIndex = sIndex;}
			catalystCpy.replace(sIndex,eIndex-sIndex+1, thisObj->executeCode()); // and execute object 
			///
			
			delete thisObj;
			thisObj = NULL; // because delete doesn't do it for us (it screws us below) 
		}
		
	} // end while() builder 
	
	
	if (thisObj == NULL && tVecVocab.size() > 0 && tVecTypes.size() > 0) {
		MethodDefinition * methDef; // do NOT delete! 
		methDef = environment->methodStructure.findMatch(tVecVocab, tVecTypes);
		if (methDef != NULL) {
			//methDef->toString();
			if (methDef->isSTDL == true) 
				thisObj = instantSTDL(&tVecVocab.at(0), NULL, environment);
			
			for (int i = 0; i < tVecVocab.size(); ++i) {
				if (thisObj->setLevelData(tVecVocab.at(i),tVecParams.at(i))) {
					if (SHOW_DEBUGGING) std::cout << "PUSHSUC: " << tVecVocab.at(i) << "() " << thisObj->getLevelData(tVecVocab.at(i)) <<std::endl; //TMP
				} else {std::cout << "CRITERROR :: Malformation: Failure to push function " << tVecVocab.at(i) << "()!" <<std::endl;exit(1);} //TMP
			}
			
		} else {
			std::cout << "CRITERROR :: Malformation: Failure to build unknown function " << tVecVocab.at(0) << "()!" <<std::endl;
			for (int i = 0; i < tVecVocab.size(); ++i) std::cout << tVecVocab.at(i) << " and " << tVecParams.at(i) << " of " << tVecTypes.at(i) <<std::endl;
			exit(1);
		}
	}
	
	
	if (thisObj != NULL) {
		// clean up //
		int sIndex=0,eIndex=0;
		sIndex = (catalystCpy.find('~'));
		eIndex = (catalystCpy.rfind('^',catalystCpy.length()-1));
		if (sIndex > eIndex) {eIndex = sIndex;}
		catalystCpy.replace(sIndex,eIndex-sIndex+1, thisObj->executeCode());
		///
		
		delete thisObj;
	}
	
	
	if (SHOW_DEBUGGING) std::cout << " OUTPUT (catcopy): " << catalystCpy <<std::endl; //TMP
	
	return catalystCpy;
}
/// ################################ ///

}
