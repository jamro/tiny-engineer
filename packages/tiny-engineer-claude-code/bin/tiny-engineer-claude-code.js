#!/usr/bin/env node
import { run } from "../src/cli.js";

await run(process.argv.slice(2));
