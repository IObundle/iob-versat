GEN_SCRIPT=gen_wave_examples.py

all: gen_svg svg2png

gen_svg:
	@./$(GEN_SCRIPT)

svg2png:
	@$(eval SVGS:=$(shell find . -maxdepth 1 -name "*.svg" ))
	@$(eval PNGS:=$(subst .svg,.png,$(SVGS)))
	@make pngs PNGS=$(PNGS)

pngs: $(PNGS)


%.png: %.svg
	@inkscape $< --export-png=$@ --export-dpi=250 --export-background=\#ffffff

clean:
	@rm -rf *.svg

deep-clean: clean
	@rm -rf *.png


