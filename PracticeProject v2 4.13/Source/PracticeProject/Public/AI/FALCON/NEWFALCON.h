// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

struct inp_field //input field
{
	float *arr_val;
	float vig_p;

	inp_field();
	~inp_field();

	void init(size_t neur_count, float _vig_p);
};

struct neuron  //neuron of the cognitive field
{
	float s_val;
	float *syn_w;  //weight vectors.

	float *act_prob; //action probabilities.
	float *act_exp_reward; //expected reward of each action.
	size_t act_idx; //for fixed neurons;

	float exp_reward; //total expected reward.

	size_t use_count;
	int32 profit_balance;

	bool bFixed;

	neuron();
	~neuron();

	void init(size_t syn_w_count, size_t act_count, float min_reward);
};

struct ActionSelectionInfo
{
	size_t selectActCounter;
	size_t successCounter;

	size_t s_count;
	float *sum_s_val;
	float *expect_s_val;

	float sum_reward;
	float expect_reward;

	ActionSelectionInfo();
	~ActionSelectionInfo();

	void init(size_t _s_count);
};

class NEWFALCON
{
	size_t act_count;

	size_t inp_field_size;
	inp_field InpF;  //S-field.

	vector <neuron*> CogF;
	size_t select_neur_idx;
	size_t select_act_idx;

	float min_reward; // minimal OK-reward.

	size_t neur_fix_lim;

	ActionSelectionInfo *SelectActStat;

	string DataFilePath;
	string InfoFilePath;
	string StatFilePath;
	string LoadDataPath;

	void add_neuron();
	void delete_neuron(size_t neur_idx);
	void clear();

	void fuzzy_and(size_t arr_size, float *arr_1, float *arr_2, float *res_arr);
	float norm(size_t arr_size, float *arr);

	size_t NeurCompetition();
	bool TemplateMatching(size_t neur_idx);
	size_t ActionSelection(size_t neur_idx); //return index of chosen action.
	void TemplateLearning(size_t neur_idx, float r);

public:
	NEWFALCON(size_t s_count, size_t a_count, float vig_p, float _min_reward, size_t _neur_fix_lim, string dataFilePath, string infoFilePath, string statFilePath, string loadDataPath);
	~NEWFALCON();

	size_t perform(float *s); //return index of chosen action.
	void learn(float r);
	void printToFile();
	void saveToFile();
	void loadFromFile();
	void printStat();
};