# my-little-dev-lab

A minimalist platform for content development and management, as well as for setting up various processes

## Fast start via docker-compose.yml

Create a `~/my-first-game/docker-compose.yml` file with the following content:
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
