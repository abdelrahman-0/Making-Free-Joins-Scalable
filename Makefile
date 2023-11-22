default: run

.PHONY: default all run plans get_data

run:
	@echo "WIP"

plans: all_plans.sh
	@chmod +x $<
	@./$<

data:
	#wget -nc http://homepages.cwi.nl/~boncz/job/imdb.tgz
	#tar xvf imdb.tgz