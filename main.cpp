#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;

struct ThreadData {
    int thread_id;
    int num_threads;
    int n, m;
    const int* A;
    const int* B;
    HANDLE hStartEvent;
    HANDLE hWritePipe;
    HANDLE hMutex;
};

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;


    WaitForSingleObject(data->hStartEvent, INFINITE);

    int result_size = data->n + data->m - 1;


    for (int k = data->thread_id; k < result_size; k += data->num_threads) {
        int coeff = 0;
        for (int i = 0; i < data->n; ++i) {
            int j = k - i;
            if (j >= 0 && j < data->m) {
                coeff += data->A[i] * data->B[j];
            }
        }


        WaitForSingleObject(data->hMutex, INFINITE);
        DWORD bytesWritten;
        int packet[2] = { k, coeff };
        WriteFile(data->hWritePipe, packet, sizeof(packet), &bytesWritten, NULL);
        ReleaseMutex(data->hMutex);
    }
    return 0;
}

int main() {
    int n, m;

    cout << "Enter the number of coefficients for the first polynomial (n): ";
    cin >> n;
    vector<int> A(n);
    cout << "Enter the coefficients of the first polynomial (space-separated): ";
    for (int i = 0; i < n; ++i) {
        cin >> A[i];
    }

    cout << "Enter the number of coefficients for the second polynomial (m): ";
    cin >> m;
    vector<int> B(m);
    cout << "Enter the coefficients of the second polynomial (space-separated): ";
    for (int i = 0; i < m; ++i) {
        cin >> B[i];
    }

    int num_threads;
    cout << "Enter the number of threads (e.g., 4): ";
    cin >> num_threads;
    if (num_threads <= 0) num_threads = 1;
    int result_size = n + m - 1;

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        cerr << "Failed to create pipe." << endl;
        return 1;
    }

    HANDLE hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);

    vector<HANDLE> threads(num_threads);
    vector<ThreadData> threadData(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        threadData[i] = { i, num_threads, n, m, A.data(), B.data(), hStartEvent, hWritePipe, hMutex };
        threads[i] = CreateThread(NULL, 0, WorkerThread, &threadData[i], 0, NULL);
    }


    SetEvent(hStartEvent);


    vector<int> C(result_size, 0);
    for (int i = 0; i < result_size; ++i) {
        int packet[2];
        DWORD bytesRead;
        ReadFile(hReadPipe, packet, sizeof(packet), &bytesRead, NULL);
        C[packet[0]] = packet[1];
    }

    WaitForMultipleObjects(num_threads, threads.data(), TRUE, INFINITE);

    cout << "Coefficients of the resulting polynomial:\n";
    for (int i = 0; i < result_size; ++i) {
        cout << C[i] << " ";
    }
    cout << "\n";

    CloseHandle(hWritePipe);
    CloseHandle(hReadPipe);
    CloseHandle(hStartEvent);
    CloseHandle(hMutex);
    for (int i = 0; i < num_threads; ++i) {
        CloseHandle(threads[i]);
    }

    return 0;
}