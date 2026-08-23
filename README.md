# my-little-dev-lab

[![my-little-dev-lab Version](https://img.shields.io/badge/my-little-dev-lab-v0.1.0-yellow.svg)](https://github.com/sea5kg/my-little-dev-lab) [![my-little-dev-lab ProgrammingLanguage](https://img.shields.io/badge/ProgrammingLanguage-c++-yellow.svg)](https://github.com/sea5kg/my-little-dev-lab) [![Docker Pulls](https://img.shields.io/docker/pulls/sea5kg/my-little-dev-lab.svg)](https://hub.docker.com/r/sea5kg/my-little-dev-lab/) [![Github Stars](https://img.shields.io/github/stars/sea5kg/my-little-dev-lab.svg?label=github%20%E2%98%85)](https://github.com/sea5kg/my-little-dev-lab/) [![Github Stars](https://img.shields.io/github/contributors/sea5kg/my-little-dev-lab.svg)](https://github.com/sea5kg/my-little-dev-lab/) [![Github Forks](https://img.shields.io/github/forks/sea5kg/my-little-dev-lab.svg?label=github%20forks)](https://github.com/sea5kg/my-little-dev-lab/)

A minimalist platform for content development and management, as well as for setting up various processes

## Fast start via docker-compose.yml

Create a `~/my-little-dev-lab/docker-compose.yml` file with the following content:
```yml
version: '3'

services:
  my_company:
    image: sea5kg/my-little-dev-lab:latest
    volumes:
      - "./data:/usr/share/mldl"
    environment:
      MLDL_DATADIR: "/usr/share/mldl"
      MLDL_USER: 1000  # automatically will be changed rights to folder and files.
      MLDL_PORT: 10233
    expose:
      - "10233"
    ports:
      - "10233:10233"
    # restart: always
    networks:
      - mldl_net

networks:
  mldl_net:
    driver: bridge
```

See: http://localhost:10233/control-panel
