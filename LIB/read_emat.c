#include "boost/algorithm/string/classification.hpp"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/trim.hpp"
#include "flexaid.h"
#include "boinc.h"

using namespace std;

void read_emat(FA_Global* FA, char* scr_bin_file)
{
	FILE* infile_ptr;
	char buffer[40];
	
	infile_ptr=NULL;
	if (!OpenFile_B(scr_bin_file,"r",&infile_ptr)){
		fprintf(stderr,"ERROR: Could not read input file: %s\n", scr_bin_file);
		Terminate(8);
	}
	
	vector<string> lines;
	string pairwiseline = "";
	
	// builds a string from a buffered line
	while(fgets(buffer,sizeof(buffer),infile_ptr) != NULL){
		pairwiseline += string(buffer);
		if(pairwiseline.find("\n") != string::npos){
			lines.push_back(pairwiseline);
			pairwiseline = "";
		}
	}
	
	printf("read %d lines in <%s>\n", (int)lines.size(), scr_bin_file);
	
	float z = zero(1.0/2.0f, 1.0/2.0f, (float)(-(int)lines.size()));
	if(fabs(z - (float)((int)z)) > 0.001) {
		fprintf(stderr,"ERROR: Number of lines read in energy matrix file <%s> is incorrect (%d)\n", 
			scr_bin_file, (int)lines.size());
		Terminate(12);
	}
	
	FA->ntypes = (int)(z + 0.001);
	printf("number of atom types: %d\n", FA->ntypes);
	
	FA->energy_matrix = (struct energy_matrix*)malloc((int)lines.size()*sizeof(struct energy_matrix));
	if(!FA->energy_matrix){
		fprintf(stderr,"ERROR: could not allocate memory for energy_matrix\n");
		Terminate(2);
	}
	
	for(int i=0;i<FA->ntypes;i++){
		for(int j=i;j<FA->ntypes;j++){
			string line = *lines.begin();
			lines.erase(lines.begin());
			
			unsigned pos = line.find("=");
			if(pos != string::npos)
				line = line.substr(pos+1);
			
			vector<string> values;
			boost::trim_if( line, boost::is_any_of(" \t\n") );
			boost::split( values, line, boost::is_any_of(" \t\n"), boost::token_compress_on );
			
			FA->energy_matrix[i*FA->ntypes+j].type1 = i;
			FA->energy_matrix[i*FA->ntypes+j].type2 = j;
			
			if(values.size() == 1){
				FA->energy_matrix[i*FA->ntypes+j].weight = 1;

				struct energy_values* weightval = (struct energy_values*)malloc(sizeof(struct energy_values));
				if(!weightval){
					fprintf(stderr,"ERROR: could not allocate memory for weightval\n");
					Terminate(2);
				}

				FA->energy_matrix[i*FA->ntypes+j].energy_values = weightval;
				
			}else if(values.size() % 2 == 0){
				FA->energy_matrix[i*FA->ntypes+j].weight = 0;
				
				struct energy_values* xyval_prev = NULL;
				
				for(vector<string>::iterator it=values.begin(); it!=values.end(); it+=2){
					vector<string>::iterator xit = it;
					vector<string>::iterator yit = it+1;
					
					struct energy_values* xyval = (struct energy_values*)malloc(sizeof(struct energy_values));
					if(!xyval){
						fprintf(stderr,"ERROR: could not allocate memory for xyval\n");
						Terminate(2);
					}
					
					xyval->pf_x = atof((*xit).c_str());
					xyval->pf_y = atof((*yit).c_str());
					xyval->next_value = NULL;
					
					// second or more xy values
					if(xyval_prev != NULL)
						xyval_prev->next_value = xyval;
					else FA->energy_matrix[i*FA->ntypes+j].energy_values = xyval; // first xy values
					
					xyval_prev = xyval;

				}
			}else{
				fprintf(stderr,"ERROR: invalid number of xy-values for atom pairwise %d-%d\n", i, j);
				Terminate(12);
			}
		}
	}


	CloseFile_B(&infile_ptr,"r");
	
}
