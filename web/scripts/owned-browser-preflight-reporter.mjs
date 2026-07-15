export default class OwnedBrowserPreflightReporter {
  onBegin(_config, suite) {
    process.stdout.write(`Owned-browser preflight started (${suite.allTests().length} test).\n`);
  }

  onTestEnd(_test, result) {
    process.stdout.write(`Owned-browser preflight test ${result.status} (${Math.round(result.duration)} ms).\n`);
  }

  onError() {
    // The browser may contain legally owned mission text. Never serialize an
    // arbitrary Playwright error, locator snapshot, page console entry, or
    // attachment into a runner log, even in the private work directory.
    process.stderr.write("Owned-browser preflight runner error (details suppressed by the content-private reporter).\n");
  }

  onEnd(result) {
    process.stdout.write(`Owned-browser preflight finished with status ${result.status}.\n`);
  }

  printsToStdio() {
    return true;
  }
}
