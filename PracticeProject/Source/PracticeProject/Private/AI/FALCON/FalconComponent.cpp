// By Polyakov Pavel

#include "PracticeProject.h"
#include "FalconComponent.h"


// Sets default values for this component's properties
UFalconComponent::UFalconComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	PrimaryComponentTick.bCanEverTick = true;

	myFALCON = new NEWFALCON(numStates * 2, 4, 0.9f, 0.1f, 20, "C:\\Users\\Admin\\Downloads\\11\\PracticeProject\\FALCON_data.txt",
        "C:\\Users\\Admin\\Downloads\\11\\PracticeProject\\FALCON_info.txt",
        "C:\\Users\\Admin\\Downloads\\11\\PracticeProject\\FALCON_stat.txt",
        "C:\\Users\\Admin\\Downloads\\11\\PracticeProject\\FALCON_data.txt");
	reward = 0;
}


// Called when the game starts
void UFalconComponent::BeginPlay()
{
	Super::BeginPlay();

	/*string ownerName = TCHAR_TO_UTF8(*GetName());
	myFALCON = new NEWFALCON(numStates * 2, 4, 0.9f, 0.1f, 20, "D:\\" + ownerName + "\\FALCON_data.txt", "D:\\" + ownerName + "\\FALCON_info.txt", "D:\\" + ownerName + "\\FALCON_stat.txt", "D:\\FALCON_for_load.txt");
	reward = 0;*/

	LoadFalcon();
}


// Called every frame
void UFalconComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	SaveFalcon();
	PrintFalcon();
	PrintStat();
}


void UFalconComponent::IncreaseReward(float r)
{
	reward += r;
}

EWeaponType UFalconComponent::PerformFalcon(TArray<float> states)
{
	float* bufS = new float[numStates * 2];
	for (int i = 0; i < numStates; i++) {
		bufS[i * 2] = states[i];
		bufS[i * 2 + 1] = 1 - states[i];
	}

	int32 retIndex = myFALCON->perform(bufS);
	delete[]bufS;

	return (EWeaponType)retIndex;
}

void UFalconComponent::LearnFalcon()
{
	myFALCON->learn(reward/MaxReward);
	reward = 0;
}

void UFalconComponent::SaveFalcon()
{
	myFALCON->saveToFile();
}

void UFalconComponent::LoadFalcon()
{
	myFALCON->loadFromFile();
}

void UFalconComponent::PrintFalcon()
{
	myFALCON->printToFile();
}

void UFalconComponent::PrintStat()
{
	myFALCON->printStat();
}
