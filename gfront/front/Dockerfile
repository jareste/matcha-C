FROM node:20

WORKDIR /app
COPY ./front /app/front

RUN npm i vite@7.1.2
RUN npm i tailwindcss @tailwindcss/vite@latest

EXPOSE 3000
CMD ["sh", "-c", "cd front/src/pages && npm run dev"]