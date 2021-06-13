#include "PracticeProject.h"
#include "NEWFALCON.h"


bool neur_comparator_s_val(neuron *n1, neuron *n2)
{
	return n1->s_val > n2->s_val;
}


inp_field::inp_field()
{
}

inp_field::~inp_field()
{
	delete[] arr_val;
}

void inp_field::init(size_t field_size, float _vig_p)
{
	arr_val = new float[field_size];
	for (size_t i = 0; i < field_size; i++) {
		arr_val[i] = 0;
	}
	vig_p = _vig_p;
}


neuron::neuron()
{
	s_val = 0;
	syn_w = nullptr;

	act_prob = nullptr;
	act_exp_reward = nullptr;
	act_idx = -1;

	exp_reward = 0;

	use_count = 0;
	profit_balance = 0;

	bFixed = false;
}

neuron::~neuron()
{
	delete[] syn_w;
	delete[] act_prob;
	delete[] act_exp_reward;
}

void neuron::init(size_t syn_w_count, size_t act_count, float min_reward)
{
	s_val = 0;
	syn_w = new float[syn_w_count];
	for (size_t i = 0; i < syn_w_count; i++) {
		syn_w[i] = 1;
	}

	act_prob = new float[act_count];
	act_exp_reward = new float[act_count];
	for (size_t i = 0; i < act_count; i++) {
		act_prob[i] = 1.f / act_count;
		act_exp_reward[i] = min_reward;
	}
	act_idx = act_count;

	exp_reward = min_reward;

	use_count = 0;
	profit_balance = 0;

	bFixed = false;
}


NEWFALCON::NEWFALCON(size_t s_count, size_t a_count, float vig_p, float _min_reward, size_t _neur_fix_lim, string dataFilePath, string infoFilePath, string statFilePath, string loadDataPath)
{
	act_count = a_count;

	inp_field_size = s_count;
	InpF.init(inp_field_size, vig_p);

	min_reward = _min_reward;

	neur_fix_lim = _neur_fix_lim;

	SelectActStat = new ActionSelectionInfo[act_count];
	for (size_t i = 0; i < act_count; i++) {
		SelectActStat[i].init(s_count);
	}

	DataFilePath = dataFilePath;
	InfoFilePath = infoFilePath;
	StatFilePath = statFilePath;
	LoadDataPath = loadDataPath;

	srand((size_t)time(nullptr));
}

NEWFALCON::~NEWFALCON()
{
	clear();

	delete[] SelectActStat;
}

size_t NEWFALCON::perform(float *s)
{
	for (size_t i = 0; i < inp_field_size; i++) {
		InpF.arr_val[i] = s[i];
	}

	select_neur_idx = NeurCompetition();
	if (CogF[select_neur_idx]->bFixed) {
		select_act_idx = CogF[select_neur_idx]->act_idx;
	}
	else {
		select_act_idx = ActionSelection(select_neur_idx);
	}

	SelectActStat[select_act_idx].selectActCounter++;
	for (size_t i = 0; i < inp_field_size; i++) {
		SelectActStat[select_act_idx].sum_s_val[i] += s[i];
		SelectActStat[select_act_idx].expect_s_val[i] = SelectActStat[select_act_idx].sum_s_val[i] / SelectActStat[select_act_idx].selectActCounter;
	}

	return select_act_idx;
}

void NEWFALCON::learn(float r)
{
	CogF[select_neur_idx]->use_count++;
	if (!CogF[select_neur_idx]->bFixed && CogF[select_neur_idx]->use_count >= neur_fix_lim) {
		float max_prob = 0;
		for (size_t i = 0; i < act_count; i++) {
			if (CogF[select_neur_idx]->act_prob[i] > max_prob) {
				max_prob = CogF[select_neur_idx]->act_prob[i];
				CogF[select_neur_idx]->act_idx = i;
			}
		}
		CogF[select_neur_idx]->bFixed = true;
	}

	if (r > 0) {
		CogF[select_neur_idx]->profit_balance++;
		SelectActStat[select_act_idx].successCounter++;
	}
	else {
		CogF[select_neur_idx]->profit_balance--;
	}

	if (CogF[select_neur_idx]->profit_balance >= 0) {
		TemplateLearning(select_neur_idx, r);
	}
	else {
		delete_neuron(select_neur_idx);
	}

	SelectActStat[select_act_idx].sum_reward += r;
	SelectActStat[select_act_idx].expect_reward = SelectActStat[select_act_idx].sum_reward / SelectActStat[select_act_idx].selectActCounter;
}

void NEWFALCON::printToFile()
{
	//ofstream output("D:\\NEWFALCON_Project_info.txt");
	ofstream output(InfoFilePath);
	for (size_t i = 0; i < CogF.size(); i++) {
		output << "neuron " << i + 1 << ": ";
		output << endl;
		output << "\texpected reward: " << CogF[i]->exp_reward << ";" << endl;
		output << "\tprofit balance: " << CogF[i]->profit_balance << ";" << endl;
		output << "\tuse count: " << CogF[i]->use_count << ";" << endl;
		output << "\tweights: ";
		for (size_t j = 0; j < inp_field_size; j++) {
			output << CogF[i]->syn_w[j] << "; ";
		}
		output << endl;
		output << "\tprobabilities: ";
		for (size_t j = 0; j < act_count; j++) {
			output << CogF[i]->act_prob[j] << "; ";
		}
		output << endl;
		output << "\tact_exp_reward: ";
		for (size_t j = 0; j < act_count; j++) {
			output << CogF[i]->act_exp_reward[j] << "; ";
		}
		output << endl;
		if (CogF[i]->bFixed) {
			output << "\tFixed: " << CogF[i]->act_idx << endl;
		}
		else {
			output << "\tNot Fixed;" << endl;
		}
	}
	output.close();
}

void NEWFALCON::saveToFile()
{
	//ofstream output("D:\\NEWFALCON_Project_data.txt");
	ofstream output(DataFilePath);
	output << act_count << endl;
	output << inp_field_size << endl;
	output << InpF.vig_p << endl;
	output << min_reward << endl;
	output << neur_fix_lim << endl;
	output << CogF.size() << endl;
	for (size_t i = 0; i < CogF.size(); i++) {
		output << CogF[i]->act_idx << endl;
		output << CogF[i]->exp_reward << endl;
		output << CogF[i]->profit_balance << endl;
		output << CogF[i]->use_count << endl;
		for (size_t j = 0; j < inp_field_size; j++) {
			output << CogF[i]->syn_w[j] << " ";
		}
		output << endl;
		for (size_t j = 0; j < act_count; j++) {
			output << CogF[i]->act_prob[j] << " ";
		}
		output << endl;
		for (size_t j = 0; j < act_count; j++) {
			output << CogF[i]->act_exp_reward[j] << " ";
		}
		output << endl;
		output << CogF[i]->bFixed << endl;
	}
	output.close();
}

void NEWFALCON::loadFromFile()
{
	clear();

	//ifstream input("D:\\NEWFALCON_Project_data.txt");
	ifstream input(LoadDataPath);

	input >> act_count;
	input >> inp_field_size;

	float _vig_p;
	input >> _vig_p;
	InpF.init(inp_field_size, _vig_p);

	input >> min_reward;
	input >> neur_fix_lim;

	size_t cogF_size;
	input >> cogF_size;
	for (size_t i = 0; i < cogF_size; i++) {
		add_neuron();
		input >> CogF[i]->act_idx;
		input >> CogF[i]->exp_reward;
		input >> CogF[i]->profit_balance;
		input >> CogF[i]->use_count;
		for (size_t j = 0; j < inp_field_size; j++) {
			input >> CogF[i]->syn_w[j];
		}
		for (size_t j = 0; j < act_count; j++) {
			input >> CogF[i]->act_prob[j];
		}
		for (size_t j = 0; j < act_count; j++) {
			input >> CogF[i]->act_exp_reward[j];
		}
		input >> CogF[i]->bFixed;
	}

	input.close();
}

void NEWFALCON::printStat()
{
	//ofstream output("D:\\NEWFALCON_Project_stat.txt");
	ofstream output(StatFilePath);
	for (size_t i = 0; i < act_count; i++) {
		output << "weapon " << i << ":" << endl;
		output << "\tselected: " << SelectActStat[i].selectActCounter << endl;
		output << "\texpected visibility percentage: " << SelectActStat[i].expect_s_val[0] << endl;
		output << "\texpected distance: " << SelectActStat[i].expect_s_val[2] << endl;
		output << "\tsuccess: " << SelectActStat[i].successCounter << endl;
		output << "\texpected reward: " << SelectActStat[i].expect_reward << endl;
	}
	output.close();
}


void NEWFALCON::add_neuron()
{
	neuron *add_n = new neuron();
	add_n->init(inp_field_size, act_count, min_reward);
	CogF.push_back(add_n);
}

void NEWFALCON::delete_neuron(size_t neur_idx)
{
	neuron *del_neur = CogF[neur_idx];
	CogF[neur_idx] = CogF[CogF.size() - 1];
	CogF.pop_back();
	delete del_neur;
}

void NEWFALCON::clear()
{
	for (size_t i = 0; i < CogF.size(); i++) {
		delete CogF[i];
	}
	CogF.clear();

	for (size_t i = 0; i < act_count; i++) {
		SelectActStat[i].init(inp_field_size);
	}
}


void NEWFALCON::fuzzy_and(size_t arr_size, float *arr_1, float *arr_2, float *res_arr)
{
	for (size_t i = 0; i < arr_size; i++) {
		res_arr[i] = fmin(arr_1[i], arr_2[i]);
	}
}

float NEWFALCON::norm(size_t arr_size, float *arr)
{
	float result = 0;
	for (size_t i = 0; i < arr_size; i++) {
		result += arr[i];
	}
	return result;
}


size_t NEWFALCON::NeurCompetition()
{
	float *and_arr = new float[inp_field_size];
	for (size_t i = 0; i < CogF.size(); i++) {
		fuzzy_and(inp_field_size, InpF.arr_val, CogF[i]->syn_w, and_arr);
		CogF[i]->s_val = norm(inp_field_size, and_arr) / norm(inp_field_size, CogF[i]->syn_w);
	}
	delete[] and_arr;

	sort(CogF.begin(), CogF.end(), neur_comparator_s_val);

	size_t win_idx = 0;
	for (size_t i = 0; i < CogF.size(); i++) {
		if (TemplateMatching(i)) {
			return win_idx;
		}
	}

	add_neuron();
	return CogF.size() - 1;
}

bool NEWFALCON::TemplateMatching(size_t neur_idx)
{
	float *and_arr = new float[inp_field_size];
	fuzzy_and(inp_field_size, InpF.arr_val, CogF[neur_idx]->syn_w, and_arr);
	float match_val = norm(inp_field_size, and_arr) / norm(inp_field_size, InpF.arr_val);
	delete[] and_arr;

	return match_val >= InpF.vig_p;
}

size_t NEWFALCON::ActionSelection(size_t neur_idx)
{
	float *arr_seg = new float[act_count + 1];

	float exp_r_sum = 0;
	for (size_t i = 0; i < act_count; i++) {
		exp_r_sum += CogF[neur_idx]->act_exp_reward[i];
	}

	float cur_exp_r_sum;
	for (size_t i = 0; i < act_count + 1; i++) {
		cur_exp_r_sum = 0;
		for (size_t j = 0; j < i; j++) {
			cur_exp_r_sum += CogF[neur_idx]->act_exp_reward[j];
		}
		arr_seg[i] = cur_exp_r_sum / exp_r_sum;
	}

	float rand_f = (float)(rand() % 100) / 100;
	size_t select_act_idx = 0;
	while (rand_f > arr_seg[select_act_idx + 1]) {
		select_act_idx++;
	}

	delete[] arr_seg;

	return select_act_idx;
}

void NEWFALCON::TemplateLearning(size_t neur_idx, float r)  //ÄÎÏÈÑÀÒÜ
{
	if (!CogF[neur_idx]->bFixed) {
		for (size_t i = 0; i < inp_field_size; i++) {
			CogF[neur_idx]->syn_w[i] = (float)(CogF[neur_idx]->use_count - 1.f) / CogF[neur_idx]->use_count * CogF[neur_idx]->syn_w[i] + 1.f / CogF[neur_idx]->use_count * InpF.arr_val[i];
		}

		//CogF[neur_idx]->act_exp_reward[select_act_idx] = (float)(CogF[neur_idx]->use_count - 1.f) / CogF[neur_idx]->use_count * CogF[neur_idx]->act_exp_reward[select_act_idx] + 1.f / CogF[neur_idx]->use_count * r;
		CogF[neur_idx]->act_exp_reward[select_act_idx] = (CogF[neur_idx]->act_exp_reward[select_act_idx] + r) / 2;
		float exp_r_sum = 0;
		for (size_t i = 0; i < act_count; i++) {
			exp_r_sum += CogF[neur_idx]->act_exp_reward[i];
		}
		for (size_t i = 0; i < act_count; i++) {
			CogF[neur_idx]->act_prob[i] = CogF[neur_idx]->act_exp_reward[i] / exp_r_sum;
		}
	}

	CogF[neur_idx]->exp_reward = (float)(CogF[neur_idx]->use_count - 1.f) / CogF[neur_idx]->use_count * CogF[neur_idx]->exp_reward + 1.f / CogF[neur_idx]->use_count * r;
}


ActionSelectionInfo::ActionSelectionInfo()
{
}

ActionSelectionInfo::~ActionSelectionInfo()
{
	delete[] sum_s_val;
	delete[] expect_s_val;
}

void ActionSelectionInfo::init(size_t _s_count)
{
	selectActCounter = 0;
	successCounter = 0;

	s_count = _s_count;
	sum_s_val = new float[s_count];
	expect_s_val = new float[s_count];
	for (size_t i = 0; i < s_count; i++) {
		sum_s_val[i] = 0;
		expect_s_val[i] = 0;
	}

	sum_reward = 0;
	expect_reward = 0;
}