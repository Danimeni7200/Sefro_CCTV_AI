def set_stream_quality(self, stream_id: str, quality: int) -> bool:
        """Set JPEG quality for a specific stream"""
        if stream_id not in self.streams:
            return False
        
        # Ensure quality is within valid range
        quality = max(1, min(100, quality))
        self.stream_qualities[stream_id] = quality
        logger.info(f"Set quality for stream {stream_id} to {quality}")
        return True