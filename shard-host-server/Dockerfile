FROM python

WORKDIR /root

RUN openssl req -new -x509 -keyout /etc/ssl/certs/server.pem -out /etc/ssl/certs/server.pem -days 365 -nodes \
	-subj "/C=CN/ST=./L=./O=./OU=../CN=."

RUN mkdir /root/serve-from-here

COPY shard-host-server.py .

CMD ["python3", "shard-host-server.py"]

