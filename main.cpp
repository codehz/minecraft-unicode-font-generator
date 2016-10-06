#include "freetype.hh"
#include <png++/png.hpp>
#include <CmdLine.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cstdio>

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

int proc(std::string fontname, unsigned fontindex, std::string target, unsigned char *glyph, unsigned base_size, unsigned font_size, int thread, int light_mode, bool pe_mode) {
	png::image<png::ga_pixel> image(base_size * 16, base_size * 16);
	char id[3] = {0};
	if (checkFTParams(light_mode)) throw std::runtime_error("Incorrect light_mode");
	FreeType<decltype(image), grayColorConverter> ft(&image, fontname, fontindex, base_size, font_size, light_mode);
	
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
		ft.genStart(i, glyph);
		sprintf(id, pe_mode ? "%02X" : "%02x", i);
		printf("\033[0;31m[%d]\033[0mgenerated \033[0;35m%s\033[0;33m(%6.2f%%)\033[0m\n", tid, id, (((float)(i - start) / (end - start)) * 100));
		image.write(target + (pe_mode ? "/glyph_" : "/unicode_page_") + id + ".png");
	}
	printf("\033[0;32m[%d]\033[0mfinsish!\n", tid);
	return !tid;
}

int main(int argc, char** argv) try {
	TCLAP::CmdLine cmd("Minecraft Unicode Font Generator", ' ', "0.3");
	TCLAP::ValueArg<std::string> fontname{"i", "input", "Input font", false, "font.ttf", "font path"};
	TCLAP::ValueArg<std::string> output{"o", "output", "Output directory", false, "out", "directory path"};
	TCLAP::ValueArg<std::string> glyph{"g", "graph", "Output glyph File", false, "glyph_sizes.bin", "file path"};
	TCLAP::ValueArg<unsigned> font_size{"f", "font_size", "Font size", false, 14, "font size"};
	TCLAP::ValueArg<unsigned> font_index{"", "font_index", "Font index", false, 0, "font index"};
	TCLAP::ValueArg<unsigned> base_size{"s", "base_size", "Unit size (must be large than Font Size)", false, 16, "unit size"};
	TCLAP::ValueArg<unsigned> thread{"t", "thread", "Thread(0 will disable this feature)", false, 0, "thread number"};
	TCLAP::MultiSwitchArg light_mode{"l", "light", "Render Text in light mode(double -l will enable mono mode)", 0};
	TCLAP::SwitchArg pe_mode("p", "pe", "Pocket Edition Mode", false);

	cmd.add(fontname);
	cmd.add(output);
	cmd.add(glyph);
	cmd.add(font_size);
	cmd.add(font_index);
	cmd.add(base_size);
	cmd.add(thread);
	cmd.add(light_mode);
	cmd.add(pe_mode);

	cmd.parse(argc, argv);

	if (base_size.getValue() < font_size.getValue()) throw std::runtime_error("Unit size must be large than Font size!");

	struct stat st = {0};
	if (stat(fontname.getValue().c_str(), &st) == -1) throw std::runtime_error("font not exist!");
	if (stat(output.getValue().c_str(), &st) == -1) mkdir(output.getValue().c_str(), 0700);

	int fd = open(glyph.getValue().c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) throw std::runtime_error("Error opening file for writing");
	unsigned char *map{0};
	try {
		if (lseek(fd, 0xFFFF - 1, SEEK_SET) == -1) throw std::runtime_error("Error calling lseek() to 'stretch' the file");
		if (write(fd, "", 1) != 1) throw std::runtime_error("Error writing last byte of the file");
		if ((map = (unsigned char *)mmap(0, 0xFFFF, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) throw std::runtime_error("Error mmapping the file");
		
	} catch (const std::exception &e) {
		close(fd);
		throw e;
	}

	if (proc(fontname.getValue(), font_index.getValue(), output.getValue(), map, base_size.getValue(), font_size.getValue(), thread.getValue(), light_mode.getValue(), pe_mode.getValue())) {
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
		close(fd);
	}

	return 0;
} catch (TCLAP::ArgException &e) {
	std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	exit(EXIT_FAILURE);
} catch (std::exception &e) {
	std::cerr << "error: " << e.what() << std::endl;
	exit(EXIT_FAILURE);
}

