FROM python:3.7-stretch

RUN apt-get update -y && \
    apt-get install -y libhunspell-dev unzip
    
RUN wget http://downloads.sourceforge.net/wordlist/hunspell-en_US-2018.04.16.zip && \
    unzip hunspell-en_US-2018.04.16.zip
    
RUN pip install hunspell
