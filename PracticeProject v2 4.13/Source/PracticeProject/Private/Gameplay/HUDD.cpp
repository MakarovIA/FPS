// By Polyakov Pavel

#include "PracticeProject.h"
#include "HUDD.h"
#include "Bot.h"
#include "Weapon.h"

AHUDD::AHUDD()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairTextureObj(TEXT("/Game/Crosshair/crosshair"));
	CrosshairTexture = CrosshairTextureObj.Object;

	static ConstructorHelpers::FObjectFinder<UFont> FontObj(TEXT("/Engine/EngineFonts/Roboto"));
	Font = FontObj.Object;
}


void AHUDD::DrawHUD() 
{
	Super::DrawHUD();
	DrawHealthbar();
	DrawWeaponName();

	if (CrosshairTexture)
	{
		// Find the center of our canvas.
		FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

		// Offset by half of the texture's dimensions so that the center of the texture aligns with the center of the Canvas.
		FVector2D CrossHairDrawPosition(Center.X - (CrosshairTexture->GetSurfaceWidth() * 0.5f), Center.Y - (CrosshairTexture->GetSurfaceHeight() * 0.5f));

		// Draw the crosshair at the centerpoint.
		FCanvasTileItem TileItem(CrossHairDrawPosition, CrosshairTexture->Resource, FLinearColor::White);
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem);
	}

}

void AHUDD::DrawHealthbar()
{
	ABot* bot = Cast<ABot>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (bot) {
		float barWidth = 200, barHeight = 50, barPad = 12, barMargin = 50;
		float percHp = bot->GetBotState().Health / bot->getMaxHealth();
		DrawRect(FLinearColor(0, 0, 0, 1), Canvas->SizeX - barWidth - barPad - barMargin, Canvas->SizeY - barHeight - barPad - barMargin, barWidth + 2 * barPad, barHeight + 2 * barPad);
		DrawRect(FLinearColor(1 - percHp, percHp, 0, 1), Canvas->SizeX - barWidth - barMargin, Canvas->SizeY - barHeight - barMargin, barWidth * percHp, barHeight);
		//Super::DrawHUD();
	}
}

void AHUDD::DrawWeaponName() 
{
	ABot* bot = Cast<ABot>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (bot) {
		FString CurrentWeaponName;
		EWeaponType CurrentWeapon = bot->GetCurrentWeapon()->GetWeaponData().WeaponType;
		switch (CurrentWeapon)
		{
		case EWeaponType::Knife:
			CurrentWeaponName = "Knife";
			break;
		case EWeaponType::Rifle:
			CurrentWeaponName = "Rifle";
			break;
		case EWeaponType::MachineGun:
			CurrentWeaponName = "Machinegun";
			break;
		case EWeaponType::Shotgun:
			CurrentWeaponName = "Shotgun";
			break;
		default:
			CurrentWeaponName = "None";
			break;
		}
		float outputWidth, outputHeight, pad = 10.f, Margin = 50.f;
		GetTextSize(CurrentWeaponName, outputWidth, outputHeight, Font, 1.5f);
		float x = Canvas->SizeX - outputWidth - pad - Margin, y = Margin;
		DrawRect(FLinearColor(0, 0, 0, 1), x, y, 2 * pad + outputWidth, 2 * pad + outputHeight);
		DrawText(CurrentWeaponName, FColor::White, x + pad, y + pad, Font, 1.5f, false);
	}
}




