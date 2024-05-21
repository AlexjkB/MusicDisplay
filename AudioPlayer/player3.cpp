#include <iostream>
#include <vector>
#include <portaudio.h>
#include <sndfile.h>
#include <cmath>
#include <fftw3.h>
#include <GLFW/glfw3.h>

#define BUFFER_SIZE 512  // Define the size of audio chunks
#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512

// Function to compute the magnitude of complex numbers
std::vector<double> computeMagnitudes(fftw_complex* result, int N) {
    std::vector<double> magnitudes(N/2+1); // N/2+1 because of symmetry in the FFT output
    for (int i = 0; i < N/2+1; ++i) {
        double real = result[i][0]; // Real part
        double imag = result[i][1]; // Imaginary part
        magnitudes[i] = sqrt(real * real + imag * imag); // Magnitude of the complex number
    }
    return magnitudes;
}

// Function to initialize GLFW and create a window
GLFWwindow* initWindow(const int resX, const int resY) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return nullptr;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    GLFWwindow* window = glfwCreateWindow(resX, resY, "Frequency Spectrum", NULL, NULL);
    
    if (window == nullptr) {
        std::cerr << "Failed to open GLFW window.\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    return window;
}

// Function to draw the frequency magnitudes
void drawMagnitudes(std::vector<double> magnitudes) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glBegin(GL_LINES);
    for (size_t i = 0; i < magnitudes.size(); ++i) {
        float x = (float)i / magnitudes.size();
        float y = magnitudes[i] / 100.0;  // Scale for visibility, adjust as needed
        glVertex2f(x, 0);
        glVertex2f(x, y);
    }
    glEnd();
}


int main() {
    SNDFILE *file;
    SF_INFO sfinfo;
    sfinfo.format = 0;
    const char *filename = "Panchiko - Sodium Chloride.flac";

    file = sf_open(filename, SFM_READ, &sfinfo);
    if (!file) {
        std::cerr << "Error: Failed to open audio file\n";
        return 1;
    }

    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (BUFFER_SIZE/2+1));
    double* in = new double[BUFFER_SIZE]; // FFT input buffer
    fftw_plan p = fftw_plan_dft_r2c_1d(BUFFER_SIZE, in, out, FFTW_MEASURE);

    PaError err = Pa_Initialize();
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream, 0, sfinfo.channels, paFloat32, sfinfo.samplerate, BUFFER_SIZE, nullptr, nullptr);
    err = Pa_StartStream(stream);

    int i = 0;
    std::vector<float> buffer(BUFFER_SIZE * sfinfo.channels);

    GLFWwindow* window = initWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!window) return -1; 

    
    std::vector<double> magnitudes(BUFFER_SIZE, 0.0);
    
    while (!glfwWindowShouldClose(window)) {
        // Process your audio and compute FFT here...
        
        while (sf_readf_float(file, buffer.data(), BUFFER_SIZE) > 0) {
            // Stream the audio
            Pa_WriteStream(stream, buffer.data(), BUFFER_SIZE);
            
            if (i % 5 == 0){
                // Copy audio data to FFT input
                for (unsigned int i = 0; i < BUFFER_SIZE; i++) {
                    in[i] = buffer[i * sfinfo.channels]; // Assuming mono or left channel for FFT
                }

                fftw_execute(p); // Perform FFT

                // Compute and optionally use magnitudes
                magnitudes = computeMagnitudes(out, BUFFER_SIZE);

                // Print the magnitudes
                //for(int i = 0; i < 32; i++) {
                //    std::cout << "Frequency bin " << i << ": " << magnitudes[i] << std::endl;
                //}
                drawMagnitudes(magnitudes);
                glfwSwapBuffers(window);
                glfwPollEvents();
            }
            ++i;
        }

    }



    fftw_destroy_plan(p);
    fftw_free(out);
    delete[] in;

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    sf_close(file);
    Pa_Terminate();

    return 0;
}

