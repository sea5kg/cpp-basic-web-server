# stage 0: build binary
FROM sea5kg/my-little-dev-lab:build-environment-2026-08-19

COPY ./ /root/
WORKDIR /root/
RUN ./build_simple.sh

# WORKDIR /root/unit-tests.wsjcpp
# RUN ./build_simple.sh
# RUN ./unit-tests

# stage 1: release
FROM sea5kg/my-little-dev-lab:release-environment-2026-08-19
LABEL "maintainer"="Evgenii Sopov <mrseakg@gmail.com>"
LABEL "repository"="https://github.com/sea5kg/my-little-dev-lab"

RUN mkdir /root/data
COPY --from=0 /root/my-little-dev-lab /usr/bin/my-little-dev-lab

EXPOSE 10233
CMD ["my-little-dev-lab"]