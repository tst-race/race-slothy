FROM python

RUN mkdir /root/scatter

WORKDIR /root/scatter

RUN apt update \
 && apt install -y libsodium-dev \
                   libjerasure-dev

ENV LIBSODIUM=/usr/lib/x86_64-linux-gnu/libsodium.so

COPY ./out/artifacts/lib/libsss.so /usr/lib/x86_64-linux-gnu/
ENV LIBSSS=/usr/lib/x86_64-linux-gnu/libsss.so

COPY ./src/scatterlib/requirements.txt ./
RUN pip install -r /root/scatter/requirements.txt

# COPY sss sss
# COPY optimize optimize
# COPY setup.py optim.py ./
COPY ./src/scatterlib/ ./scatterlib
COPY README.md ./

RUN pip install ./scatterlib

CMD ["/bin/bash"]
