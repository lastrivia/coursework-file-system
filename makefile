all:
	make -C step1
	make -C step2
	make -C step3

submit:
	mkdir -p submit
	cp -r src submit/
	mkdir -p submit/step1 submit/step2 submit/step3
	cp step1/*.cpp step1/makefile submit/step1
	cp step2/*.cpp step2/makefile submit/step2
	cp step3/*.cpp step3/makefile submit/step3
	cp -f makefile Prj3README typescript.md submit/
	tar -cvf submit.tar submit/*

clean:
	make -C step1 clean
	make -C step2 clean
	make -C step3 clean
	rm -rf submit submit.tar