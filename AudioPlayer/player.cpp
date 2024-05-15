#include <iostream>
#include <vector>
#include <portaudio.h>
#include <sndfile.h>

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

    // Print vector to view information

    // Initialize PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Open an audio stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream, 0, sfinfo.channels, paFloat32, sfinfo.samplerate, paFramesPerBufferUnspecified, nullptr, nullptr);
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

    // Write audio data to the stream
    err = Pa_WriteStream(stream, audioData.data(), sfinfo.frames);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    // Wait for the stream to finish playing
    while (Pa_IsStreamActive(stream)) {
        Pa_Sleep(100);
    }

    // Stop and close the audio stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // Terminate PortAudio
    Pa_Terminate();

    return 0;
}
