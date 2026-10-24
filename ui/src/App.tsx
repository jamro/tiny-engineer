import { Route, Switch } from "wouter-preact";
import { Footer } from "./components/Footer";
import { Nav } from "./components/Nav";
import { StatusBar } from "./components/StatusBar";
import { ConfigPage } from "./config/ConfigPage";
import { AuthGate } from "./gates/AuthGate";
import { RebootGate } from "./gates/RebootGate";
import { RobotProvider, useRobot } from "./robot";
import { StatusProvider } from "./status/status";
import { Animations } from "./views/Animations";
import { ApiReference } from "./views/ApiReference";
import { Extensions } from "./views/Extensions";
import { Home } from "./views/Home";
import { ServoControl } from "./views/ServoControl";
import { Tests } from "./views/Tests";

function Shell() {
  const { gate, inSetup } = useRobot();

  if (gate === "booting") {
    return null;
  }

  if (gate === "auth") {
    return <AuthGate />;
  }

  if (gate === "reboot") {
    return <RebootGate />;
  }

  if (inSetup) {
    return (
      <>
        <StatusBar />
        <ConfigPage />
      </>
    );
  }

  return (
    <>
      <Nav />
      <StatusBar />
      <Switch>
        <Route path="/animations" component={Animations} />
        <Route path="/tests" component={Tests} />
        <Route path="/servo" component={ServoControl} />
        <Route path="/extensions" component={Extensions} />
        <Route path="/config" component={ConfigPage} />
        <Route path="/api" component={ApiReference} />
        <Route component={Home} />
      </Switch>
      <Footer />
    </>
  );
}

export function App() {
  return (
    <StatusProvider>
      <RobotProvider>
        <Shell />
      </RobotProvider>
    </StatusProvider>
  );
}
