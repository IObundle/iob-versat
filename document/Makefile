# SPDX-FileCopyrightText: 2025 IObundle
#
# SPDX-License-Identifier: MIT

# Makefile for creating documents in the build directory.
# Adapted from py2hwsw's generic document/Makefile (py2hwsw/document/Makefile),
# which is the same Makefile shared by every IObundle core's document/ folder.

SHELL = bash

# include core basic info
ROOT_DIR :=..
include ../config_build.mk

#include local doc makefile segment
ifneq ($(wildcard doc_build.mk),)
include doc_build.mk
endif

#default DOC
DOC?=ds

PDFLATEX = pdflatex -interaction=nonstopmode

CONFIDENTIAL?=1

TSRC = $(patsubst tsrc/%, ./%, $(wildcard tsrc/*))
%.tex: tsrc/%.tex
	cp $< $@
%.cls: tsrc/%.cls
	cp $< $@
%.py: tsrc/%.py
	cp $< $@
%.h: tsrc/%.h
	cp $< $@
%.c: tsrc/%.c
	cp $< $@

TSRC+=name.tex

TSRC+=signature.tex
signature.tex: $(DOC)_signature.tex
	mv $< $@

pb_signature.tex ds_signature.tex:
	@echo `tail -n 1 name.tex`-USG-`tail -n 1 $(NAME)_version.tex`-`tail -n 1 shortHash.tex` > $@

rn_signature.tex:
	@echo `tail -n 1 name.tex`-REL-`tail -n 1 $(NAME)_version.tex`-`tail -n 1 shortHash.tex` > $@


#make figures
figs:
	make -C ./figures figs

pb_figs:
	make -C ./figures pb_figs

#doxygen documentation
ifeq ($(DOXYGEN),1)
TSRC+=doxygen.tex
doxygen.tex: doxygen
	cp doxygen/latex/*.tex .
	cp doxygen/latex/*.sty .
	cp doxygen/*.sty . # copy custom .sty files after generated doxygen .sty
	cp doxygen/doxygen.tex $@

doxygen:
	make -C ./doxygen all
endif

#build document
build: debug $(DOC).pdf

#build presentation document type
presentation.pdf: figs presentation.tex
	$(PDFLATEX) -jobname pres presentation.tex
	$(PDFLATEX) -jobname pres presentation.tex

#build product brief document type
pb.pdf: debug results pb_figs pbtop.tex
	$(PDFLATEX) -jobname pb pbtop.tex
	$(PDFLATEX) -jobname pb pbtop.tex

#pb top file containing the definition of sections to include
pbtop.tex:
	make doctop DOC=pb

#build design specification document type
ds.pdf: debug results figs dstop.tex
	$(PDFLATEX) -jobname ds dstop.tex
	$(PDFLATEX) -jobname ds dstop.tex
	$(PDFLATEX) -jobname ds dstop.tex

#ds top file containing the definition of sections to include
dstop.tex:
	if ls *_py_params_tab.tex > /dev/null 2>&1; then echo "\def\PYPARAMS{Y}" >> $@; fi
	if ls *_confs_tab.tex > /dev/null 2>&1; then echo "\def\CONFS{Y}" >> $@; fi
	if [ -f td.tex ]; then echo "\def\TD{Y}" >> $@; fi
	if ls *_csrs_tab.tex > /dev/null 2>&1; then echo "\def\CSRS{Y}" >> $@; fi
	if [ -f mems_tab.tex ]; then echo "\def\MEMS{Y}" >> $@; fi
	if [ $(DOXYGEN) == 1 ]; then echo "\def\DOXYGEN{Y}" >> $@; fi
	if ls linux.tex > /dev/null 2>&1; then echo "\def\LINUX{Y}" >> $@; fi
	if [ $(CONFIDENTIAL) == 1 ]; then echo "\def\CONFIDENTIAL{Y}" >> $@; fi
	make doctop DOC=ds

#include implementation results if available

ifneq ($(wildcard quartus.tex),)
INTEL_FPGA:=1
endif

ifneq ($(wildcard vivado.tex),)
AMD_FPGA:=1
endif

RESULTS = $(INTEL_FPGA)$(AMD_FPGA)$(UMC130_ASIC)

rntop.tex:
	make doctop DOC=rn

rn.pdf: rntop.tex
	$(PDFLATEX) -jobname rn rntop.tex
	$(PDFLATEX) -jobname rn rntop.tex


doctop:
	echo "\def\NAME{$(NAME)}" >> $(DOC)top.tex
ifeq ($(DOC),rn)
	echo "\def\RN{Y}" >> $(DOC)top.tex
endif
	if [ $(SECTIONCLEARPAGE) ]; then echo "\def\SECTIONCLEARPAGE{Y}" >> $(DOC)top.tex; fi
	if [ $(RESULTS) ]; then echo "\def\RESULTS{Y}" >> $(DOC)top.tex; fi
	if [ $(INTEL_FPGA)$(AMD_FPGA) ]; then echo "\def\FPGA{Y}" >> $(DOC)top.tex; fi
	if [ $(AMD_FPGA) ]; then echo "\def\AMD{Y}" >> $(DOC)top.tex; fi
	if [ $(INTEL_FPGA) ]; then echo "\def\INTEL{Y}" >> $(DOC)top.tex; fi
	if [ $(ASIC_NODE) ]; then echo "\def\ASIC{Y}" >> $(DOC)top.tex; fi
	if [ -f custom.tex ]; then echo "\def\CUSTOM{Y}" >> $(DOC)top.tex; fi
	echo "\input{$(DOC).tex}" >> $(DOC)top.tex


#view document
view: $(DOC).pdf
	evince $< &

clean:
	@find . -maxdepth 1 -type f -not \( -name \*.pdf -o -name Makefile -o -name \*.mk -o -name $(NAME)\* \) -delete
	@rm -rf $(PYTHON_DIR)/__pycache__

veryclean: clean
	make -C figures clean

test: clean debug pb.pdf clean debug ds.pdf clean debug
	if [ -f tsrc/presentation.tex ]; then make presentation.pdf; fi

debug: $(TSRC)
	@echo TSRC=$(TSRC)
	@echo RESULTS=$(RESULTS)
	@echo FPGA=$(FPGA)
	@echo INTEL_FPGA=$(INTEL_FPGA)
	@echo AMD_FPGA=$(AMD_FPGA)
	@echo ASIC_NODE=$(ASIC_NODE)

.PHONY: view doctop figs pb_figs results build clean veryclean test debug doxygen
