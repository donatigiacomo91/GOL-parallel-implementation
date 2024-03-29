sequential:
	icc sequential.cpp -std=c++11 -O3 -o sequential.exe -finline-functions

m_sequential:
	icc sequential.cpp -std=c++11 -O3 -o m_sequential.exe -finline-functions -mmic

pthread:
	icc pthread.cpp -std=c++11 -O3 -pthread -o pthread.exe -finline-functions

m_pthread:
	icc pthread.cpp -std=c++11 -O3 -pthread -o m_pthread.exe -finline-functions -mmic

omp:
	icc omp.cpp -std=c++11 -O3 -fopenmp -o omp.exe -finline-functions

m_omp:
	icc omp.cpp -std=c++11 -O3 -fopenmp -o m_omp.exe -finline-functions -mmic

fastflow:
	icc fastflow.cpp -std=c++11 -O3 -pthread -I /home/spm1501/fastflow -o fastflow.exe -DNO_DEFAULT_MAPPING -finline-functions

m_fastflow:
	icc fastflow.cpp -std=c++11 -O3 -pthread -I /home/spm1501/fastflow -o m_fastflow.exe -DNO_DEFAULT_MAPPING -finline-functions -mmic

tester:
	icc tester.cpp -std=c++11 -O3 -o tester.exe

compile:
	make sequential
	make m_sequential
	make pthread
	make m_pthread
	make omp
	make m_omp
	make fastflow
	make m_fastflow

move0:
	scp m_sequential.exe mic0:
	scp m_pthread.exe mic0:
	scp m_omp.exe mic0:
	scp m_fastflow.exe mic0:

move1:
	scp m_sequential.exe mic1:
	scp m_pthread.exe mic1:
	scp m_omp.exe mic1:
	scp m_fastflow.exe mic1:

clean:
	rm *.exe