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
#include <type_traits>
#include <tuple>

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

struct FontPack {
    png::image<png::ga_pixel> image;
    FreeType<png::image<png::ga_pixel>, grayColorConverter> ft;
    unsigned base_size;

    FontPack(std::string fontname, unsigned fontindex, unsigned base_size, unsigned font_size, int light_mode) : image(base_size * 16, base_size * 16) ,ft(&image, fontname, fontindex, base_size, font_size, light_mode), base_size(base_size) {
    }
};

void generate(const FontPack &pack, int i, unsigned char *glyph, const std::string &filename) {
    auto [image, ft, base_size] = pack;
    clearImage(image, base_size * 16);
    ft.genStart(i, glyph);
    image.write(filename);
}

int proc(const FontPack &pack, std::string target, unsigned char *glyph, int thread, bool pe_mode) {
	char id[3] = {0};
	
	int thrd = thread;
	int splice = 0xFF / (thread + 1);
	int tid = 0;

	while (thrd-- > 0) if (fork() == 0) {
		tid = thread - thrd;
		printf("\033[0;32m[%d]\033[0mstarting\n", tid);
		break;
	}

	if (!tid) printf("\033[0;32m[0]\033[0mstarting\n");

	int start = tid * splice;
	int end = ((tid == thread) ? 0xFF : ((tid + 1) * splice - 1));

	std::string filename_prefix = target + (pe_mode ? "/glyph_" : "/unicode_page_");
	
	for (int i = start; i <= end; i++) {
		sprintf(id, pe_mode ? "%02X" : "%02x", i);
	    generate(pack, i, glyph, filename_prefix + id + ".png");
		printf("\033[0;31m[%d]\033[0mgenerated \033[0;35m%s\033[0;33m(%6.2f%%)\033[0m\n", tid, id, (((float)(i - start) / (end - start)) * 100));
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
	TCLAP::ValueArg<int> preview{"v", "preview", "Generate a preview page(from specific code point)", false, -1, "code point"};

	cmd.add(fontname);
	cmd.add(output);
	cmd.add(glyph);
	cmd.add(font_size);
	cmd.add(font_index);
	cmd.add(base_size);
	cmd.add(thread);
	cmd.add(light_mode);
	cmd.add(pe_mode);
	cmd.add(preview);

	cmd.parse(argc, argv);

	if (base_size.getValue() < font_size.getValue()) throw std::runtime_error("Unit size must be large than Font size!");

	struct stat st = {0};
	if (stat(fontname.getValue().c_str(), &st) == -1) throw std::runtime_error("font not exist!");

	int fd = open(glyph.getValue().c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) throw std::runtime_error("Error opening file for writing");

	FontPack fontPack{fontname.getValue(), font_index.getValue(), base_size.getValue(), font_size.getValue(), light_mode.getValue()};

	if (preview.getValue() > -1) {
	    std::cout << "\033[0;32mGenerating Preview...\033[0m" << std::endl;
	    generate(fontPack, preview.getValue(), nullptr, output.getValue() + "_preview.png");
	    std::cout << "\033[0;32mPreview Image Generated.\033[0m" << std::endl;
	    //_exit(0);
	    return 0;
	}

	if (stat(output.getValue().c_str(), &st) == -1) mkdir(output.getValue().c_str(), 0700);

	unsigned char *map{0};
	try {
		if (lseek(fd, 0xFFFF - 1, SEEK_SET) == -1) throw std::runtime_error("Error calling lseek() to 'stretch' the file");
		if (write(fd, "", 1) != 1) throw std::runtime_error("Error writing last byte of the file");
		if ((map = (unsigned char *)mmap(0, 0xFFFF, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) throw std::runtime_error("Error mmapping the file");
	} catch (const std::exception &e) {
		close(fd);
		throw e;
	}

	std::cout << "\033[0;32mSTARTING\033[0m" << std::endl;

	if (proc(fontPack, output.getValue(), map, thread.getValue(), pe_mode.getValue())) {
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
		printf("\033[0;32mALL DONE!\033[0m\n");
		close(fd);
	}

	//_exit(0);
	return 0;
} catch (TCLAP::ArgException &e) {
	std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	exit(EXIT_FAILURE);
} catch (FTError &e) {
	std::cerr << "FTError: " << e.what() << std::endl;
	exit(EXIT_FAILURE);
} catch (std::exception &e) {
	std::cerr << "error: " << e.what() << std::endl;
	exit(EXIT_FAILURE);
} catch (...) {
	std::cerr << "unknown error" << std::endl;
	exit(EXIT_FAILURE);
}

