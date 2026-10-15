import { HandAssignment } from "../components/HandAssignment";
import { useRobot } from "../robot";
import { Wizard } from "../setup/Wizard";
import { ConfigForm } from "./ConfigForm";
import { FactoryReset } from "./FactoryReset";

export function ConfigPage() {
  const { inSetup, settings } = useRobot();

  if (inSetup) {
    return <section>{settings && <Wizard settings={settings} />}</section>;
  }

  return (
    <section>
      <h2 className="page-title">Config</h2>
      <p className="page-desc">Saved to flash. Most changes apply right away.</p>
      {settings && (
        <>
          <ConfigForm settings={settings} />
          <HandAssignment settings={settings} />
          <FactoryReset />
        </>
      )}
    </section>
  );
}
