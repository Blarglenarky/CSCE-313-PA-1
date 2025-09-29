/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Jesse Ashen
	UIN: 833008069
	Date: 9/16/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int m = MAX_MESSAGE;
	bool new_chan = false;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	//kick off server
	//give arguments for server
	//m is max number of bytes that can be sent between server and client
	//server needs './server', '-m', '<val for -m arg>', 'NULL'
	//fork
	//in child run execvp using server arguments
	//////////////////////////////////////////////////////////////////////////////////
	pid_t pid = fork();
	if(pid<0){//if it fails
		perror("fork failed");
		exit(1);
	}
	if(pid==0){//if child, replace itself by execvp
		char path[] = "./server";
		string m_val = to_string(m);
		char* args[] = {path, (char*)"-m", (char*)m_val.c_str(), NULL};
		if(execvp(args[0], args) < 0){
			perror("execvp failed");
			exit(1);
		}
	}
	///////////////////////////////////////////////////////////////////////////////////


	FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);


	FIFORequestChannel* def_channel = &chan;

	if (new_chan) {
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		chan.cwrite(&nc, sizeof(MESSAGE_TYPE));

		char new_name[100];
		chan.cread(new_name, sizeof(new_name));

		def_channel = new FIFORequestChannel(new_name, FIFORequestChannel::CLIENT_SIDE);
		cout << "New channel created: " << new_name << endl;
	}

	system("mkdir -p received");

	//single data point only when p, t, e != -1
	// example data point request
    //char buf[MAX_MESSAGE]; // 256
	if(t != 0.0 && filename == ""){
		datamsg x(p, t, e); //user values patient, time, egc1/2

		
		//memcpy(buf, &x, sizeof(datamsg));
		def_channel->cwrite(&x, sizeof(datamsg)); // question
		double reply;
		def_channel->cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}



	//if p != -1, request 1000 datapoints
	//loop over first 1000 lines 
	//send request for ecg1, send request fro ecg2
	//writ line for recieved/x1.csv
	//format is; time, ecg1, ecg2
	if(p>0 && t == 0.0 && filename == ""){
		ofstream outfile("received/x1.csv");
		double t_curr =0.0;
		for(int i = 0; i < 1000; i++){
			//get ecg1 values
			datamsg d1(p, t_curr, 1);
			def_channel->cwrite(&d1, sizeof(datamsg));
			double val1;
			def_channel->cread(&val1, sizeof(double));

			//get ecg2 values
			datamsg d2(p, t_curr, 2);
			def_channel->cwrite(&d2, sizeof(datamsg));
			double val2;
			def_channel->cread(&val2, sizeof(double));

			outfile << t_curr << "," << val1 << "," << val2 << endl; //prints to return message properly formatted
			t_curr += 0.004; //increments time
		}
		outfile.close(); //close file
	}



    // sending a non-sense message, you need to change this
	// filemsg fm(0, 0);
	// string fname = "teslkansdlkjflasjdf.dat";//replace this with use given -f argument
	
	if (filename != ""){
		filemsg fm(0,0);
		string fname = filename;
		int len = sizeof(filemsg) + (filename.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), filename.c_str());


		def_channel->cwrite(buf2, len);  // I want the file length;

		int64_t filesize = 0; //She initalizes to 0 in the video
		def_channel->cread(&filesize, sizeof(int64_t));

		char* buf3 = new char[m];//create buffer of size buff capcaity m

		std::ofstream outfile;
		outfile.open("received/" + filename, std::ios::binary);
		// string pathout = "received/" + filename;
		// ofstream outfile(pathout, ios::binary);


		

		// //loop over the segments in the file filesize / buff capacity
		// //create file message instance
		// filemsg* file_req = (filemsg*)buf2;
		// file_req->offset = //set offset in file
		// file_req->length = //set the length. Be careful of the last signal
		// //send the request buf2
		// chan.cwrite(buf2, len);
		// //recieve the response
		// //cread into buf3 length file_req->len
		// //write buf3 into file: recieved/filename

		int64_t offset = 0;
		int64_t remaining = filesize;
		std::cout << " Getting here";

		while(remaining > 0){
			int chunk = min((int64_t)m, remaining);
			filemsg* filemsg2 = (filemsg*)buf2;
			filemsg2->offset = offset;
			filemsg2->length = chunk;

			cout << filemsg2->offset << " " << filemsg2->length << " " << remaining << endl;
			def_channel->cwrite(buf2, len);
			def_channel->cread(buf3, chunk);
			outfile.write(buf3, chunk);

			offset += chunk;
			remaining -= chunk;
		}

		delete[] buf2;
		delete[] buf3;
		outfile.close();
		

	}


	// closing the channel    
    MESSAGE_TYPE quit = QUIT_MSG;
    def_channel->cwrite(&quit, sizeof(MESSAGE_TYPE));

	if(new_chan){
		delete def_channel;
		chan.cwrite(&quit, sizeof(MESSAGE_TYPE));
	}
}



