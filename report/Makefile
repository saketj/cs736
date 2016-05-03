MAIN      = main
SRCS      = *.tex 
TARG      = $(MAIN).pdf


all: $(TARG)

pdf: $(SRCS) $(MAIN).bbl
	source ./SourceMe ; \
	pdflatex $(MAIN)
	source ./SourceMe ; \
	pdflatex $(MAIN)
	#cp main.pdf ~/Dropbox/Documents/

$(TARG): $(SRCS) $(MAIN).bbl
	source ./SourceMe ; \
	pdflatex $(MAIN)
	source ./SourceMe ; \
	pdflatex $(MAIN)
	touch $(TARG)
	#cp main.pdf ~/Dropbox/Documents/

bib: $(MAIN).bbl bib.tex

$(MAIN).bbl: $(MAIN).aux
	source ./SourceMe ; \
	bibtex -min-crossrefs=100 $(MAIN)
	./redo-bib.csh $(MAIN)

$(MAIN).aux: $(MAIN).tex
	source ./SourceMe ; \
	pdflatex $(MAIN)

clean:
	rm -f $(TARG) *.aux *~ $(MAIN).log $(MAIN).bbl $(MAIN).blg

.SECONDARY: $(DVI)
