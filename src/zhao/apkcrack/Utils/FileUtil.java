package zhao.apkcrack.Utils;

public class FileUtil {
	public static native String deodex(String src, String dst);
	public static native String oat2dex(String src, String dst);
	private static native boolean testExecute(String file);
	public static native String zipalign(String src, String dst, int mode);
}