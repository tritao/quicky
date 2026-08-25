// Small shared helpers for the non-generated QUIKY Ghidra scripts.
//
// This deliberately contains project mechanics only.  Function names,
// ranges, ABI notes, and probe targets remain in their respective scripts or
// in the authoritative closure manifest.

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Program;

public final class QuikyGhidra {
    private static final Pattern SEGMENT_NAME = Pattern.compile(".*_SEG(\\d+)(?:\\..*)?");

    private QuikyGhidra() {
    }

    public static int segmentNumber(String programName) {
        Matcher matcher = SEGMENT_NAME.matcher(programName);
        if (!matcher.matches()) {
            throw new IllegalArgumentException(
                "program name has no _SEGNN suffix: " + programName);
        }
        return Integer.parseInt(matcher.group(1));
    }

    public static Address address(Program program, long offset) {
        return program.getAddressFactory().getDefaultAddressSpace().getAddress(offset);
    }

    public static String safeProgramFilename(String programName) {
        return programName.replaceAll("[^A-Za-z0-9_.-]", "_");
    }

    public static String jsonEscape(String value) {
        if (value == null) return "";
        return value.replace("\\", "\\\\").replace("\"", "\\\"");
    }
}
