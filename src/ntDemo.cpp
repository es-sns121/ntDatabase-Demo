/*
 * =======================================================================================
 *
 *  ntTest.cpp
 *
 *	Source file for specific normative type demos and demonstrations.
 *
 *	The following normative types are "demoed" and there functionality 
 *	is shown. 
 *		NTScalar
 *		NTEnum
 *		NTURI
 *		NTNameValue
 *		NTTable
 *		NTAttribute
 *		NTMultiChannel
 *
 *	The remaining normative types were added to the database to demonstrate their
 *	functionality. The methods required to interact with them are the same as the 
 *	ones demonstrated here, so for brevity and due to the relative increased complexity 
 *	(in the sense they have numerous fields to have to populate and demo) of the 
 *	remaining normative types they are excluded.
 *		NTNDArray
 *		NTContinuum
 *		NTHistogram
 *		NTAggregate
 *
 * ========================================================================================
 */

#include "ntDemo.h"
#include "ntScalarDemo.h"
#include <pv/pvAccess.h>
#include <pv/pvaClient.h>
#include <pv/pvData.h>
#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>

using namespace std;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvaClient;

/* Formatted print function for demo function results */
void printResult(const bool &result, const string &channel_name) {
	
	if (result) {
		cout << channel_name << " record demo successful\n";
	} else {
		cout << channel_name << " record demo unsuccessful\n";
	}

	return;
}

/* Wrapper function that calls specific demo functions */
int demoRecord(
	bool verbosity,
	PvaClientPtr pva,
	string const & channel_name)
{
	bool result(false);
	
	static bool first_call = true;

	/* map of function pointers keyed on channel name */
	static map<string, bool (*) (bool, PvaClientChannelPtr)> functions;
	
	map<string, bool (*) (bool, PvaClientChannelPtr)>::iterator it;

	if (true == first_call) {
	/* init functions map */
		first_call = false;
		functions["string"] = &demoString;
		functions["stringArray"] = &demoStringArray;
		functions["short"] = &demoShort;
		functions["shortArray"] = &demoShortArray;
		functions["int"] = &demoInt;
		functions["intArray"] = &demoIntArray;
		functions["long"] = &demoLong;
		functions["longArray"] = &demoLongArray;
		functions["double"] = &demoDouble;
		functions["doubleArray"] = &demoDoubleArray;
		functions["enum"] = &demoEnum;
		functions["matrix"] = &demoMatrix;
		functions["uri"] = &demoURI;
		functions["name_value"] = &demoNameValue;
		functions["table"] = &demoTable;
		functions["attribute"] = &demoAttribute;
	}

	if (channel_name.compare("multi_channel") == 0) {
		result = demoMultiChannel(verbosity, pva, channel_name);
		printResult(result, channel_name);
		return 0;
	}

	it = functions.find(channel_name);
	
	if (functions.end() == it) {
		
		cerr << "Channel '" << channel_name << "' not recognised.\n";
		return 1;
	
	} else {	
	
		PvaClientChannelPtr channel = pva->channel(channel_name);
		
		if (channel) cout << "\nChannel \"" << channel_name << "\" connected succesfully\n";
		else
			return 1;
		
		result = (it->second)(verbosity, channel);
		printResult(result, channel_name);
	
	}

	return 0;

}

/* NTEnum demonstration */
bool demoEnum(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);

	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	PVStructurePtr pvStructure = putData->getPVStructure();
	// Request copy of the choices array in the enum
	shared_vector<const string> choices = pvStructure->getSubField<PVStringArray>("value.choices")->view();
	// Get current index of choices array in the enum
	int read = pvStructure->getSubField<PVInt>("value.index")->get();
	
	stringstream out;
	
	out << "\n\tBefore write: enum(zero, one) = " << choices[read] << endl;
	
	
	// Write a new index to the record.
	int write = 1;
	pvStructure->getSubField<PVInt>("value.index")->put(write);

	putGet->putGet();

	// Check that it changed.
	pvStructure = getData->getPVStructure();
	read = pvStructure->getSubField<PVInt>("value.index")->get();

	out << "\t After write: enum(zero, one) = " << choices[read] << endl << endl;

	if (read != write)
		result = false;
	
	if (verbosity)
		cout << out.str();

	return result;
}

/* NTMatrix demonstration */
bool demoMatrix(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);

	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	// Dimensions of matrix, 5 x 5 for a total of 25 cells.
	shared_vector<int> dim_data(2);
	dim_data[0] = 5; dim_data[1] = 5;
	
	// Values to be inserted into matrix.
	shared_vector<double> val_data(25);
	for (int i = 0; i < 25; ++i) val_data[i] = (double) i;
	
	shared_vector<const int> dim(freeze(dim_data));
	shared_vector<const double> value(freeze(val_data));
	// Note that the data vectors are now empty.
	
	// Replace the data in the record's vectors and flush the changed bitsets to the server.
	putData->getPVStructure()->getSubField<PVIntArray>("dim")->replace(dim);	
	putData->getPVStructure()->getSubField<PVDoubleArray>("value")->replace(value);	
	putGet->putGet();

	// Read the data from the record and check that it is correct.
	shared_vector<const int> read_dim;
	shared_vector<const double> read_val;

	putGet->getGetData();
	read_dim = getData->getPVStructure()->getSubField<PVIntArray>("dim")->view();
	read_val = getData->getPVStructure()->getSubField<PVDoubleArray>("value")->view();

	stringstream out;

	/* These for loops print out the matrix's dimensions and then contents in a grid. */
	out << "\n\tMatrix: ";
	out << "dim:";
	for (size_t i = 0; i < read_dim.size(); ++i) {
		out << " " << read_dim[i];
		if (read_dim[i] != dim[i])
			result = false;
	}
	
	out << "\n\t        val:";
	for (size_t i = 0, j = 0; i < read_val.size(); ++i, ++j) {
		if (j == (unsigned) dim[1]) {
			out << "\n\t            ";
			j = 0;
		}
		out << " " << setw(4) << read_val[i];
		if (read_val[i] != value[i])
			result = false;
	}
	out << "\n\n";

	if (verbosity)
		cout << out.str();

	return result;
}

/* NTURI demonstration */
bool demoURI(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);

	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	string scheme_write = "URI scheme string";
	string path_write = "URI path string";
	string query_write = "URI query string";
	
	// Write data to the record.
	putData->getPVStructure()->getSubField<PVString>("scheme")->put(scheme_write);
	putData->getPVStructure()->getSubField<PVString>("path")->put(path_write);
	putData->getPVStructure()->getSubField<PVString>("query.query")->put(query_write);
	putGet->putGet();

	string scheme_read;
	string path_read;
	string query_read;

	/* Get data from the server and read */
	putGet->getGetData();	
	scheme_read = getData->getPVStructure()->getSubField<PVString>("scheme")->get();
	path_read = getData->getPVStructure()->getSubField<PVString>("path")->get();
	query_read = getData->getPVStructure()->getSubField<PVString>("query.query")->get();
	
	/* Print before and after */
	stringstream out;
	out << "\n\twrite\n";
	out << "\t\t" << setw(8) << "scheme: " << scheme_write << endl;
	out << "\t\t" << setw(8) << "path: " << path_write << endl;
	out << "\t\t" << setw(8) << "query: " << query_write << endl;
	

	out << "\tread\n";
	out << "\t\t" << setw(8) << "scheme: " << scheme_read << endl;
	out << "\t\t" << setw(8) << "path: " << path_read << endl;
	out << "\t\t" << setw(8) << "query: " << query_read << endl << endl;

	if (scheme_write != scheme_read ||
		path_write != path_read ||
		query_write != query_read)
		result = false;

	if (verbosity)
		cout << out.str();

	return result;
}

/* NTNameValue demonstration */
bool demoNameValue(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);

	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	// Create a vector of names for the record.
	shared_vector<string> name_data;
	name_data.push_back("one");
	name_data.push_back("two");
	name_data.push_back("three");
	shared_vector<const string> name(freeze(name_data));
	
	// Create a vector of values for the record.
	shared_vector<double> value_data;
	value_data.push_back(1);
	value_data.push_back(2);
	value_data.push_back(3);
	shared_vector<const double> value(freeze(value_data));
	
	// Write the names and values to the record.
	putData->getPVStructure()->getSubField<PVStringArray>("name")->replace(name);
	putData->getPVStructure()->getSubField<PVDoubleArray>("value")->replace(value);
	putGet->putGet();

	// Read the names and values from the record.
	putGet->getGetData();
	shared_vector<const string> name_read = getData->getPVStructure()->getSubField<PVStringArray>("name")->view();
	shared_vector<const double> value_read = getData->getPVStructure()->getSubField<PVDoubleArray>("value")->view();
	
	stringstream out;
	out << "\n\t names:";
	for(size_t i = 0; i < name_read.size(); ++i) {
		out << right << setw(8) << name_read[i];
		if (name[i] != name_read[i])
			result = false;
	}
	out << "\n\tvalues:";
	for(size_t i = 0; i < value_read.size(); ++i) {
		out << setw(8) << value_read[i];
		if (value[i] != value_read[i])
			result = false;
	}
	out << "\n\n";
	
	if (verbosity)
		cout << out.str();

	return result;
}

/* NTTable demonstration */
bool demoTable(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);
	
	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	// Create the questions vector
	shared_vector<string> data;
	data.push_back("Why are we here?");
	data.push_back("How are we to be happy?");
	data.push_back("Whats the meaning to life?");
	data.push_back("Whats the answer to the ultimate question of life, the universe, and everything?");
	shared_vector<const string> questions(freeze(data));
	
	// Create the answers vector.
	data.push_back("42");
	shared_vector<const string> answers(freeze(data));
	
	// Create the recommendations vector
	data.push_back("Keep calm.");
	data.push_back("Always carry a towel.");
	data.push_back("Drink heavily and read the guide.");
	shared_vector<const string> recommendations(freeze(data));
	
	// Write the vectors to the table record. These consitute the tables columns
	putData->getPVStructure()->getSubField<PVStringArray>("value.questions")->replace(questions);
	putData->getPVStructure()->getSubField<PVStringArray>("value.answers")->replace(answers);
	putData->getPVStructure()->getSubField<PVStringArray>("value.recommendations")->replace(recommendations);
	putGet->putGet();

	putGet->getGetData();
	shared_vector<const string> labels
		= getData->getPVStructure()->getSubField<PVStringArray>("labels")->view();
	
	shared_vector<const string> questions_read
		= getData->getPVStructure()->getSubField<PVStringArray>("value.questions")->view();
	
	shared_vector<const string> answers_read
		= getData->getPVStructure()->getSubField<PVStringArray>("value.answers")->view();
	
	shared_vector<const string> recommendations_read
		= getData->getPVStructure()->getSubField<PVStringArray>("value.recommendations")->view();

	stringstream out;
	
	// Print out the read vectors and check for data consistency between write/read

	out << "\n\t" << setw(17) << "labels:";
	for (size_t i = 0; i < labels.size(); ++i) {
		out << " " << labels[i];
	}
	out << "\n\t" << setw(17) << "questions:";	
	for (size_t i = 0; i < questions_read.size(); ++i) {
		out << " " << questions_read[i];
		if (questions[i] != questions_read[i]) 
			result = false;
	}
	out << "\n\t" << setw(17) << "answers:";	
	for (size_t i = 0; i < answers_read.size(); ++i) {
		out << "  " << answers_read[i];
		if (answers[i] != answers_read[i]) 
			result = false;
	}
	out << "\n\t" << setw(17) << "recommendations:";	
	for (size_t i = 0; i < recommendations_read.size(); ++i) {
		out << " " << recommendations_read[i];
		if (recommendations[i] != recommendations_read[i]) 
			result = false;
	}
	out << "\n\n";

	if (verbosity)
		cout << out.str();
	
	return result;
}

/* NTAttribute demonstration */
bool demoAttribute(
	bool verbosity,
	PvaClientChannelPtr channel)
{
	bool result(true);

	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();
	
	string name   = "The ultimate answer";
	// Create a PVType to pass to the union.
	PVDataCreatePtr pvDataCreate = epics::pvData::getPVDataCreate();
	PVStringPtr answer = pvDataCreate->createPVScalar<PVString>();
	answer->put("42");
	// Use the set function to set the union to the pv.
	putData->getPVStructure()->getSubField<PVString>("name")->put(name);
	putData->getPVStructure()->getSubField<PVUnion>("value")->set(answer);

	putGet->putGet();	

	putGet->getGetData();

	string name_read = getData->getPVStructure()->getSubField<PVString>("name")->get();
	string answer_read = getData->getPVStructure()->getSubField<PVUnion>("value")->get<PVString>()->get();

	stringstream out;
	out << "\n\t name: " << name_read;
	if (name_read != name) result = false;
	out << "\n\tvalue: " << answer_read;
	if (answer_read != answer->get()) result = false;
	out << "\n\n";

	if (verbosity)
		cout << out.str();

	return result;
}

/* NTMultiChannel Demo */
bool demoMultiChannel(
	bool verbosity,
	PvaClientPtr pva,
	const string & channel_name)
{
	bool result(true);
	
	PvaClientChannelPtr channel = pva->channel(channel_name);
	
	if (channel) cout << "\nChannel \"" << channel_name << "\" connected succesfully\n";
	else
		return false;
	
	// Create putGet to read and write to/from record.
	PvaClientPutGetPtr putGet = channel->createPutGet("");
	PvaClientPutDataPtr putData = putGet->getPutData();
	PvaClientGetDataPtr getData = putGet->getGetData();

	// Open two channels to pvRecords currently hosted on an accessible database. You must know that these exist.
	// We're going to use two records that are already populated in our database.
	PvaClientChannelPtr channel_long   = pva->channel("long");
	
	bool long_connect = (channel_long) ? true : false;

	PvaClientGetPtr long_get = channel_long->createGet();
	PvaClientGetDataPtr long_data = long_get->getData();
	
	PvaClientChannelPtr channel_double = pva->channel("double");
	
	bool double_connect = (channel_double) ? true : false;
	
	result = (long_connect && double_connect) ? true : false;
	
	PvaClientGetPtr double_get = channel_double->createGet();
	PvaClientGetDataPtr double_data = double_get->getData();

	// Create the channel names vector to populate the record with.
	shared_vector<string> channelNames_data(2);
	channelNames_data[0] = "long"; 
	channelNames_data[1] = "double";
	shared_vector<const string> channelNames(freeze(channelNames_data));

	// Create the values vector to populate the record with. 
	// These will be the values currently held in the two connected records.
	PVDataCreatePtr pvDataCreate = epics::pvData::getPVDataCreate();
	shared_vector<PVUnionPtr> value_data(2);
	value_data[0] = pvDataCreate->createPVVariantUnion();
	value_data[1] = pvDataCreate->createPVVariantUnion();	
	
	value_data[0]->set(long_data->getPVStructure()->getSubField<PVLong>("value"));
	value_data[1]->set(double_data->getPVStructure()->getSubField<PVDouble>("value"));
	shared_vector<const PVUnionPtr> value(freeze(value_data));

	// Create the connect status vector to populate the record with.
	shared_vector<char> isConnected_data(2);
	isConnected_data[0] = long_connect;
	isConnected_data[1] = double_connect;
	shared_vector<const char> isConnected(freeze(isConnected_data));
	
	putData->getPVStructure()->getSubField<PVStringArray>("channelName")->replace(channelNames);
	putData->getPVStructure()->getSubField<PVUnionArray>("value")->replace(value);
	putData->getPVStructure()->getSubField<PVBooleanArray>("isConnected")->replace(isConnected);
	
	putGet->putGet();
	putGet->getGetData();

	if (verbosity)
		cout << getData->getPVStructure() << endl;

	return result;
}
