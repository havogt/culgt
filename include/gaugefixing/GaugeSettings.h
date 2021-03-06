/**
 * GaugeSettings.h
 *
 *  Created on: Apr 23, 2014
 *      Author: vogt
 */

#ifndef GAUGESETTINGS_H_
#define GAUGESETTINGS_H_

#include <boost/program_options/options_description.hpp>

namespace culgt
{

class GaugeSettings
{
public:
	int getCheckPrecision() const
	{
		return checkPrecision;
	}

	void setCheckPrecision(int checkPrecision)
	{
		this->checkPrecision = checkPrecision;
	}

	int getGaugeCopies() const
	{
		return gaugeCopies;
	}

	void setGaugeCopies(int gaugeCopies)
	{
		this->gaugeCopies = gaugeCopies;
	}

	bool isRandomTrafo() const
	{
		return randomTrafo;
	}

	void setRandomTrafo(bool randomTrafo)
	{
		this->randomTrafo = randomTrafo;
	}

	int getOrMaxIter() const
	{
		return orMaxIter;
	}

	void setOrMaxIter(int orMaxIter)
	{
		this->orMaxIter = orMaxIter;
	}

	float getOrParameter() const
	{
		return orParameter;
	}

	void setOrParameter(float orParameter)
	{
		this->orParameter = orParameter;
	}

	double getPrecision() const
	{
		return precision;
	}

	void setPrecision(double precision)
	{
		this->precision = precision;
	}

	bool isPrintStats() const
	{
		return printStats;
	}

	void setPrintStats(bool printStats)
	{
		this->printStats = printStats;
	}

	int getReproject() const
	{
		return reproject;
	}

	void setReproject(int reproject)
	{
		this->reproject = reproject;
	}

	float getSaMax() const
	{
		return saMax;
	}

	void setSaMax(float saMax)
	{
		this->saMax = saMax;
	}

	float getSaMin() const
	{
		return saMin;
	}

	void setSaMin(float saMin)
	{
		this->saMin = saMin;
	}

	int getSaSteps() const
	{
		return saSteps;
	}

	void setSaSteps(int saSteps)
	{
		this->saSteps = saSteps;
	}

	int getMicroiter() const
	{
		return microiter;
	}

	void setMicroiter( int microiter )
	{
		this->microiter = microiter;
	}

	bool isSaLog() const
	{
		return saLog;
	}

	void setSaLog(bool saLog)
	{
		this->saLog = saLog;
	}

	const string& getFileAppendix() const {
		return fileAppendix;
	}

	void setFileAppendix(const string& fileAppendix) {
		this->fileAppendix = fileAppendix;
	}

	bool isSethot() const {
		return sethot;
	}

	void setSethot(bool sethot) {
		this->sethot = sethot;
	}

	int getTuneFactor() const {
		return tuneFactor;
	}

	void setTuneFactor(int tuneFactor) {
		this->tuneFactor = tuneFactor;
	}

	boost::program_options::options_description getGaugeOptions()
	{
		boost::program_options::options_description gaugeOptions("Gaugefixing options");

		gaugeOptions.add_options()
					("gaugecopies", boost::program_options::value<int>(&gaugeCopies)->default_value(1), "calculate <arg> gauge copies and save the one with best functional value")
					("randomtrafo", boost::program_options::value<bool>(&randomTrafo)->default_value(true), "apply a random gauge transformation before gauge fixing")

					("precision", boost::program_options::value<double>(&precision)->default_value(1e-14), "desired precision")
					("checkprecision", boost::program_options::value<int>(&checkPrecision)->default_value(100), "check if precision is reached every <arg> step")
					("reproject", boost::program_options::value<int>(&reproject)->default_value(100), "reproject links to SU(N) every <arg> step")

					("ormaxiter", boost::program_options::value<int>(&orMaxIter)->default_value(100), "maximal number of overrelaxation iterations")
					("orparameter", boost::program_options::value<float>(&orParameter)->default_value(1.7), "overrelaxation parameter w (g^w)")

					("samax", boost::program_options::value<float>(&saMax)->default_value(1.4), "Simulated Annealing start temperature")
					("samin", boost::program_options::value<float>(&saMin)->default_value(0.1), "Simulated Annealing end temperature")
					("sasteps", boost::program_options::value<int>(&saSteps)->default_value(0), "number of Simulated Annealing steps")
					("salog", boost::program_options::value<bool>(&saLog)->default_value(false), "decrease the temperature logarithmically")

					("microiter", boost::program_options::value<int>(&microiter)->default_value(3), "number of microcanonical updates per heatbath in Simulated Annealing")

					("printstats", boost::program_options::value<bool>(&printStats)->default_value(true), "print progress on command line")

					("sethot", boost::program_options::value<bool>(&sethot)->default_value(false), "start from a random gauge field")
					("tunefactor", boost::program_options::value<int>(&tuneFactor)->default_value(5), "factor to influence the number of tuning steps")
					("fappendix", boost::program_options::value<string>(&fileAppendix)->default_value("gaugefixed_"), "file appendix (append after basename when writing)")
					;


		return gaugeOptions;
	}

private:

	double precision;

	int saSteps;
	float saMax;
	float saMin;
	bool saLog;

	int microiter;

	int orMaxIter;
	float orParameter;

	int checkPrecision;
	int reproject;

	int gaugeCopies;

	bool randomTrafo;

	bool printStats;

	bool sethot;
	int tuneFactor;
	string fileAppendix;
};

}

#endif /* GAUGESETTINGS_H_ */
