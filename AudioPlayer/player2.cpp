#include <iostream>
#include <vector>
#include <portaudio.h>
#include <sndfile.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>
#include <math.h>

// Constants
const int BUFFER_SIZE = 1024; // Buffer size for real-time processing

// Global variables for audio buffer
std::vector<float> audioBuffer(BUFFER_SIZE);

// Function to perform FFT
void performFFT(const std::vector<float>& audioBuffer, std::vector<float>& amplitudeSpectrum) {
    int N = audioBuffer.size();
    fftw_complex* in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex* out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; ++i) {
        in[i][0] = audioBuffer[i]; // Real part
        in[i][1] = 0.0;            // Imaginary part
    }

    fftw_execute(plan);

    amplitudeSpectrum.resize(N / 2);
    for (int i = 0; i < N / 2; ++i) {
        amplitudeSpectrum[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}

// Function to calculate amplitude
float calculateAmplitude(const std::vector<float>& audioBuffer) {
    float sum = 0.0f;
    for (float sample : audioBuffer) {
        sum += sample * sample;
    }
    return sqrt(sum / audioBuffer.size());
}

// OpenGL error callback
void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "GL Debug Output: " << message << std::endl;
}

// OpenGL shaders
const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform float amplitude;
void main() {
    gl_Position = vec4(aPos.x, aPos.y * amplitude, aPos.z, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 baseColor;
uniform float intensity;
void main() {
    FragColor = vec4(baseColor * intensity, 1.0);
}
)";

// Audio callback
int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
    const std::vector<float>* audioData = (const std::vector<float>*)userData;
    float *out = (float*)outputBuffer;
    static size_t offset = 0;

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        out[i] = audioData->at(offset + i);
    }
    offset += framesPerBuffer;

    return paContinue;
}

int main() {
    SNDFILE *file;
    SF_INFO sfinfo;
    sfinfo.format = 0;
    const char *filename = "Panchiko - Sodium Chloride.flac";

    // Open the audio file
    file = sf_open(filename, SFM_READ, &sfinfo);
    if (!file) {
        std::cerr << "Error: Failed to open audio file\n";
        return 1;
    }

    // Get metadata
    const char *title = sf_get_string(file, SF_STR_TITLE);
    const char *artist = sf_get_string(file, SF_STR_ARTIST);
    const char *album = sf_get_string(file, SF_STR_ALBUM);
    const char *year = sf_get_string(file, SF_STR_DATE);
    const char *tracknumber = sf_get_string(file, SF_STR_TRACKNUMBER);

    // Print metadata
    std::cout << "Metadata found:\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Title: " << (title ? title : "N/A") << '\n';
    std::cout << "Artist: " << (artist ? artist : "N/A") << '\n';
    std::cout << "Album: " << (album ? album : "N/A") << '\n';
    std::cout << "Year: " << (year ? year : "N/A") << '\n';
    std::cout << "Track Number: " << (tracknumber ? tracknumber : "N/A") << '\n';
    std::cout << "----------------------------------------\n"; 

    // Allocate memory for audio data
    std::vector<float> audioData(sfinfo.frames * sfinfo.channels);

    // Read audio data from the file
    sf_readf_float(file, audioData.data(), sfinfo.frames);

    // Close the audio file
    sf_close(file);

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Open an audio stream with a callback
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream, 0, sfinfo.channels, paFloat32, sfinfo.samplerate, BUFFER_SIZE, audioCallback, &audioData);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    // Initialize GLFW and create a window
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Visualization", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewInit();

    // Enable OpenGL debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(glDebugOutput, nullptr);

    // Setup OpenGL resources (VAOs, VBOs, Shaders)
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, audioData.size() * sizeof(float), audioData.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Capture audio data
        std::vector<float> currentBuffer(BUFFER_SIZE * sfinfo.channels);
        if (audioData.size() >= currentBuffer.size()) {
            std::copy(audioData.begin(), audioData.begin() + currentBuffer.size(), currentBuffer.begin());
        }

        // Perform FFT and calculate amplitude
        std::vector<float> amplitudeSpectrum;
        performFFT(currentBuffer, amplitudeSpectrum);
        float amplitude = calculateAmplitude(currentBuffer);

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update shader with audio data
        glUseProgram(shaderProgram);
        glUniform1f(glGetUniformLocation(shaderProgram, "amplitude"), amplitude);
        glUniform3f(glGetUniformLocation(shaderProgram, "baseColor"), 0.2f, 0.6f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "intensity"), amplitude);

        // Render visualization
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, audioData.size() / sfinfo.channels);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup OpenGL resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Close and terminate PortAudio
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    glfwTerminate();

    return 0;
}

