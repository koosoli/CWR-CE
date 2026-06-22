# Build context is mserver/ (so the papa-bear-client path dependency is present).
# mserver/.dockerignore trims the context to just the two Rust crates.
#
#   docker build -t <registry>/papa-bear/master-service:<tag> \
#     -f docker/papa-bear-master-service/Dockerfile mserver

FROM docker.io/library/rust:1-bookworm AS builder
WORKDIR /app
COPY Client ./Client
COPY MasterService ./MasterService
RUN cargo build --release --locked \
        --manifest-path MasterService/Cargo.toml --bin papa-bear-master-service \
    && strip MasterService/target/release/papa-bear-master-service

FROM gcr.io/distroless/cc-debian12:nonroot
COPY --from=builder /app/MasterService/target/release/papa-bear-master-service /usr/local/bin/papa-bear-master-service
# State lives on a mounted volume; the chart sets fsGroup so nonroot can write here.
VOLUME ["/data"]
EXPOSE 8080
ENTRYPOINT ["/usr/local/bin/papa-bear-master-service"]
CMD ["server", "--listen", "0.0.0.0:8080", "--db", "/data/papa-bear.sqlite3"]
