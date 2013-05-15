PREFIX = "/usr/"
all: nbc c

install: 
	@echo installing executable files to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -vf src/nbc/probabilities-by-read ${DESTDIR}${PREFIX}/bin/probabilities-by-read
	@cp -vf src/nbc/count ${DESTDIR}${PREFIX}/bin/count-kmers
	@cp -vf src/c/quikr_train ${DESTDIR}${PREFIX}/bin/quikr_train
	@cp -vf src/c/quikr ${DESTDIR}${PREFIX}/bin/quikr
	@cp -vf src/c/multifasta_to_otu ${DESTDIR}${PREFIX}/bin/multifasta_to_otu 
	@cp -vf src/python/generate_kmers ${DESTDIR}${PREFIX}/bin/generate_kmers
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/probabilities-by-read
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/count-kmers
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/quikr
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/quikr_train
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/multifasta_to_otu
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/generate_kmers
	
nbc:
	@echo "building nbc"
	@cd src/nbc; make

c:
	@echo "building c"
	@cd src/c; make
python:
	@echo "configuring python"
	@cd src/python; python setup.py build

install_python:
	@cd src/python; python setup.py install
	@cp -vf src/nbc/probabilities-by-read ${DESTDIR}${PREFIX}/bin/probabilities-by-read
	@cp -vf src/nbc/count ${DESTDIR}${PREFIX}/bin/count-kmers
	@cp -vf src/python/quikr ${DESTDIR}${PREFIX}/bin/quikr.py
	@cp -vf src/python/quikr_train ${DESTDIR}${PREFIX}/bin/quikr_train.py
	@cp -vf src/python/multifasta_to_otu ${DESTDIR}${PREFIX}/bin/multifasta_to_otu.py
	@cp -vf src/python/generate_kmers ${DESTDIR}${PREFIX}/bin/generate_kmers
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/probabilities-by-read
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/count-kmers
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/quikr.py
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/quikr_train.py
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/multifasta_to_otu.py
	chmod -v 755 ${DESTDIR}${PREFIX}/bin/generate_kmers

clean:
	@echo "cleaning up"
	@cd src/python; rm build -Rvf
	@cd src/nbc; make clean
	@cd src/c; make clean
