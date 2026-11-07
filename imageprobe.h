#pragma once
#include <QDateTime>
#include <QFileInfo>
#include <QImageReader>
#include <QVariantMap>
namespace ImageProbe {
inline QVariantMap inspect(const QString &path, QString *error) {
  if (error)
    error->clear();
  auto fail = [&](const QString &s) {
    if (error)
      *error = s;
    return QVariantMap{};
  };
  QFileInfo f(path);
  if (!f.isFile() || f.isSymLink() || !f.isReadable())
    return fail("文件不存在、不可读或为符号链接");
  if (f.size() <= 0 || f.size() > 50LL * 1024 * 1024)
    return fail("文件为空或超过50MiB限制");
  QImageReader reader(f.canonicalFilePath());
  reader.setDecideFormatFromContent(true);
  reader.setAutoTransform(true);
  if (!reader.canRead())
    return fail("文件内容不是受支持的图片");
  const auto size = reader.size();
  if (!size.isValid() || qint64(size.width()) * size.height() > 100000000)
    return fail("尺寸无效或超过1亿像素限制");
  const QString format = QString::fromLatin1(reader.format()).toUpper();
  if(!QStringList{"PNG","JPEG","JPG","BMP","WEBP"}.contains(format))return fail("不支持该图片内容格式");
  if (size.width() > 256 || size.height() > 256)
    reader.setScaledSize(size.scaled(256, 256, Qt::KeepAspectRatio));
  if (reader.read().isNull())
    return fail("图片解码预检失败：" + reader.errorString());
  return {
      {"path", f.canonicalFilePath()},
      {"width", size.width()},
      {"height", size.height()},
      {"bytes", f.size()},
      {"format", format},
      {"modified_at", f.lastModified().toUTC().toString(Qt::ISODateWithMs)}};
}
} // namespace ImageProbe
