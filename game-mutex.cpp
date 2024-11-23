#include "icb_gui.h"
#include <stdlib.h>
#include <time.h>
#include <windows.h>


HANDLE HMutex;
int MutexFlag = 0;


int FRM1;
int keypressed;
int boxlocation = 10;  
int boxY = 380;        
int bulletX, bulletY;  
bool bulletActive = false;
int fallingBoxX, fallingBoxY = 0; 
int fallingBoxWidth;  

// Parçacıklar için diziler
#define MAX_FRAGMENTS 15
int fragmentX[MAX_FRAGMENTS];  
int fragmentY[MAX_FRAGMENTS];  
int fragmentDX[MAX_FRAGMENTS]; 
int fragmentDY[MAX_FRAGMENTS]; 
bool fragmentActive[MAX_FRAGMENTS]; 

bool isShattered = false;  
int shatterDuration = 20;  

bool thread_continue_sliding = false;
bool thread_continue_falling = false;
ICBYTES m;

void ICGUI_Create() {

    ICG_MWTitle("Shooting Falling Boxes Game");
    ICG_MWSize(450, 500);
    srand(time(NULL)); 
}

void InitializeShatter(int direction) {
    for (int i = 0; i < MAX_FRAGMENTS; ++i) {
        fragmentX[i] = fallingBoxX + (rand() % fallingBoxWidth);
        fragmentY[i] = fallingBoxY + (rand() % fallingBoxWidth);
        fragmentDX[i] = (rand() % 3 + 1) * (direction == 0 ? (rand() % 2 ? 1 : -1) : direction);
        fragmentDY[i] = -(rand() % 5 + 2);
        fragmentActive[i] = true;
    }
    isShattered = true;
    shatterDuration = 20;
}

void UpdateShatter() {
    bool anyActive = false;

    for (int i = 0; i < MAX_FRAGMENTS; ++i) {
        if (fragmentActive[i]) {
            FillRect(m, fragmentX[i], fragmentY[i], 5, 5, 0x000000);  // Eski pozisyonu temizle

            fragmentX[i] += fragmentDX[i];
            fragmentY[i] += fragmentDY[i];

            if (fragmentX[i] >= 0 && fragmentX[i] < 400 && fragmentY[i] >= 0 && fragmentY[i] < 400) {
                FillRect(m, fragmentX[i], fragmentY[i], 5, 5, 0xff0000);
                anyActive = true;
            }
            else {
                fragmentActive[i] = false;
            }
        }
    }

    if (--shatterDuration <= 0 || !anyActive) {
        for (int i = 0; i < MAX_FRAGMENTS; ++i) {
            FillRect(m, fragmentX[i], fragmentY[i], 5, 5, 0x000000);
            fragmentActive[i] = false;
        }
        isShattered = false;

        fallingBoxX = rand() % (450 - fallingBoxWidth);
        fallingBoxY = 0;
    }
}

DWORD WINAPI SlidingBox(LPVOID lpParam) {
    while (thread_continue_sliding) {
        FillRect(m, boxlocation, boxY, 20, 6, 0x000000);
        if (keypressed == 37 && boxlocation > 0) boxlocation -= 5;
        if (keypressed == 39 && boxlocation < 430) boxlocation += 5;
        FillRect(m, boxlocation, boxY, 20, 6, 0xff0000);
        DisplayImage(FRM1, m);
        Sleep(30);
    }
    return 0;
}

void FireBullet() {
    if (!bulletActive) {
        bulletX = boxlocation + 10;
        bulletY = 370;
        bulletActive = true;
    }
}

DWORD WINAPI FallingBox(LPVOID lpParam) {
    while (thread_continue_falling) {
        WaitForSingleObject(HMutex, INFINITE);

        if (isShattered) {
            UpdateShatter();
        }
        else {
            FillRect(m, fallingBoxX, fallingBoxY, fallingBoxWidth, fallingBoxWidth, 0x000000);

            fallingBoxY += 5;
            if (fallingBoxY > 380) {
                fallingBoxX = rand() % (450 - fallingBoxWidth);
                fallingBoxY = 0;
            }
        }

        if (bulletActive) {
            FillRect(m, bulletX, bulletY, 5, 10, 0x000000);
            bulletY -= 5;

            if (bulletY < 0) {
                bulletActive = false;
            }
            else if (bulletY <= fallingBoxY + fallingBoxWidth && bulletY >= fallingBoxY) {
                int relativeX = bulletX - fallingBoxX;

                if (relativeX >= 0 && relativeX <= fallingBoxWidth) {
                    PlaySound(TEXT("step.wav"), NULL, SND_FILENAME | SND_ASYNC);

                    if (relativeX <= 3 * (fallingBoxWidth / 10)) {
                        InitializeShatter(1);  // Left section hit: Shatter to the right
                    }
                    else if (relativeX >= 7 * (fallingBoxWidth / 10)) {
                        InitializeShatter(-1); // Right section hit: Shatter to the left
                    }
                    else {
                        InitializeShatter(0);  // Middle section hit: Shatter upward
                    }

                    bulletActive = false;
                }
            }

            if (bulletActive) {
                FillRect(m, bulletX, bulletY, 5, 10, 0x00ff00);
            }
        }

        // Draw the falling box if it is not shattered
        if (!isShattered) {
            FillRect(m, fallingBoxX, fallingBoxY, fallingBoxWidth, fallingBoxWidth, 0x0000ff);
        }

        // Release the mutex before displaying and sleeping
        ReleaseMutex(HMutex);

        // Display the updated frame
        DisplayImage(FRM1, m);
        Sleep(30);
    }
    return 0;
}



void WhenKeyPressed(int k) {
    keypressed = k;
    if (keypressed == 32) FireBullet();  // Space tuşu ile ateş et
}

void butonfonk() {
    static HANDLE hThreadSlidingBox = NULL;
    static HANDLE hThreadFallingBox = NULL;

    if (!thread_continue_sliding && !thread_continue_falling) {
        thread_continue_sliding = true;
        thread_continue_falling = true;
        hThreadSlidingBox = CreateThread(NULL, 0, SlidingBox, NULL, 0, NULL);
        hThreadFallingBox = CreateThread(NULL, 0, FallingBox, NULL, 0, NULL);
        SetFocus(ICG_GetMainWindow());
    }
    else {
        thread_continue_sliding = false;
        thread_continue_falling = false;
        WaitForSingleObject(hThreadSlidingBox, INFINITE);
        WaitForSingleObject(hThreadFallingBox, INFINITE);
        CloseHandle(hThreadSlidingBox);
        CloseHandle(hThreadFallingBox);
    }
}

void ICGUI_main() {
    ICG_Button(5, 5, 120, 25, "Start / Stop", butonfonk);
    FRM1 = ICG_FrameMedium(5, 40, 400, 400);
    ICG_SetOnKeyPressed(WhenKeyPressed);

    fallingBoxX = rand() % (450 - fallingBoxWidth);
    fallingBoxWidth = (rand() % 5 + 1) * 10;

    CreateImage(m, 400, 400, ICB_UINT);

    // Mutex oluşturma
    HMutex = CreateMutex(NULL, FALSE, NULL);
}
