#include "freetype.hh"
#include <png++/png.hpp>
#include <CmdLine.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

struct grayColorConverter {
	png::ga_pixel operator () (unsigned char color) {
		return png::ga_pixel(0xFF, color);
	}
};

template <typename T>
void clearImage(T &image, unsigned N) {
	grayColorConverter converter;
	for (unsigned i = 0; i < N; i++)
		for (unsigned j = 0; j < N; j++)
			image[i][j] = converter((unsigned char)0);
}

int proc(std::string fontname, std::string target, unsigned base_size, unsigned font_size, int thread = 0) {
	png::image<png::ga_pixel> image(base_size * 16, base_size * 16);
	char id[3] = {0};
	FreeType<decltype(image), grayColorConverter> ft(&image, fontname, 0, base_size, font_size);
	
	int thrd = thread;
	int splice = 0xFF / (thread + 1);
	int tid = 0;

	while (thrd-- > 0) if (fork() == 0) {
		tid = thread - thrd;
		break;
	}

	int start = tid * splice;
	int end = ((tid == thread) ? 0xFF : ((tid + 1) * splice - 1));
	
	for (int i = start; i <= end; i++) {
		clearImage(image, base_size * 16);
		ft.genStart(i);
		sprintf(id, "%02x", i);
		printf("\033[0;31m[%d]\033[0mgenerated \033[0;35m%s\033[0;33m(%6.2f%%)\033[0m\n", tid, id, (((float)(i - start) / (end - start)) * 100));
		image.write(target + "/unicode_page_" + id + ".png");
	}
	printf("\033[0;32m[%d]\033[0mfinsish!\n", tid);
	return !tid;
}

int main(int argc, char** argv) try {
	TCLAP::CmdLine cmd("Minecraft Unicode Font Generator", ' ', "0.1");
	TCLAP::ValueArg<std::string> fontname{"i", "input", "Input font", false, "font.ttf", "font path"};
	TCLAP::ValueArg<std::string> output{"o", "output", "Output directory", false, "out", "directory path"};
	TCLAP::ValueArg<unsigned> font_size{"f", "font_size", "Font size", false, 14, "font size"};
	TCLAP::ValueArg<unsigned> base_size{"s", "base_size", "Unit size (must be large than Font Size)", false, 16, "unit size"};
	TCLAP::ValueArg<unsigned> thread{"t", "thread", "Thread(0 will disable this feature)", false, 0, "thread number"};

	cmd.add(fontname);
	cmd.add(output);
	cmd.add(font_size);
	cmd.add(base_size);
	cmd.add(thread);

	cmd.parse(argc, argv);

	if (base_size.getValue() < font_size.getValue()) throw std::runtime_error("Unit size must be large than Font size!");

	struct stat st = {0};
	if (stat(fontname.getValue().c_str(), &st) == -1) throw std::runtime_error("font not exist!");
	if (stat(output.getValue().c_str(), &st) == -1) mkdir(output.getValue().c_str(), 0700);

	if (proc(fontname.getValue(), output.getValue(), base_size.getValue(), font_size.getValue(), thread.getValue())) {
		while (true) {
			int status;
			pid_t done = wait(&status);
			if (done == -1) {
				if (errno == ECHILD) break; // no more child processes
			} else {
				if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
					throw std::runtime_error("sub process failed");
			}
		}
	}

	return 0;
} catch (TCLAP::ArgException &e) {
	std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
} catch (FTError &e) {
	std::cerr << "error: " << e.what() << std::endl;
} catch (std::exception &e) {
	std::cerr << "error: " << e.what() << std::endl;
}

